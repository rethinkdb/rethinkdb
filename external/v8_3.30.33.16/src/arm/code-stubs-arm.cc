// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/v8.h"

#if V8_TARGET_ARCH_ARM

#include "src/base/bits.h"
#include "src/bootstrapper.h"
#include "src/code-stubs.h"
#include "src/codegen.h"
#include "src/ic/handler-compiler.h"
#include "src/ic/ic.h"
#include "src/isolate.h"
#include "src/jsregexp.h"
#include "src/regexp-macro-assembler.h"
#include "src/runtime/runtime.h"

namespace v8 {
namespace internal {


static void InitializeArrayConstructorDescriptor(
    Isolate* isolate, CodeStubDescriptor* descriptor,
    int constant_stack_parameter_count) {
  Address deopt_handler = Runtime::FunctionForId(
      Runtime::kArrayConstructor)->entry;

  if (constant_stack_parameter_count == 0) {
    descriptor->Initialize(deopt_handler, constant_stack_parameter_count,
                           JS_FUNCTION_STUB_MODE);
  } else {
    descriptor->Initialize(r0, deopt_handler, constant_stack_parameter_count,
                           JS_FUNCTION_STUB_MODE, PASS_ARGUMENTS);
  }
}


static void InitializeInternalArrayConstructorDescriptor(
    Isolate* isolate, CodeStubDescriptor* descriptor,
    int constant_stack_parameter_count) {
  Address deopt_handler = Runtime::FunctionForId(
      Runtime::kInternalArrayConstructor)->entry;

  if (constant_stack_parameter_count == 0) {
    descriptor->Initialize(deopt_handler, constant_stack_parameter_count,
                           JS_FUNCTION_STUB_MODE);
  } else {
    descriptor->Initialize(r0, deopt_handler, constant_stack_parameter_count,
                           JS_FUNCTION_STUB_MODE, PASS_ARGUMENTS);
  }
}


void ArrayNoArgumentConstructorStub::InitializeDescriptor(
    CodeStubDescriptor* descriptor) {
  InitializeArrayConstructorDescriptor(isolate(), descriptor, 0);
}


void ArraySingleArgumentConstructorStub::InitializeDescriptor(
    CodeStubDescriptor* descriptor) {
  InitializeArrayConstructorDescriptor(isolate(), descriptor, 1);
}


void ArrayNArgumentsConstructorStub::InitializeDescriptor(
    CodeStubDescriptor* descriptor) {
  InitializeArrayConstructorDescriptor(isolate(), descriptor, -1);
}


void InternalArrayNoArgumentConstructorStub::InitializeDescriptor(
    CodeStubDescriptor* descriptor) {
  InitializeInternalArrayConstructorDescriptor(isolate(), descriptor, 0);
}


void InternalArraySingleArgumentConstructorStub::InitializeDescriptor(
    CodeStubDescriptor* descriptor) {
  InitializeInternalArrayConstructorDescriptor(isolate(), descriptor, 1);
}


void InternalArrayNArgumentsConstructorStub::InitializeDescriptor(
    CodeStubDescriptor* descriptor) {
  InitializeInternalArrayConstructorDescriptor(isolate(), descriptor, -1);
}


#define __ ACCESS_MASM(masm)


static void EmitIdenticalObjectComparison(MacroAssembler* masm,
                                          Label* slow,
                                          Condition cond);
static void EmitSmiNonsmiComparison(MacroAssembler* masm,
                                    Register lhs,
                                    Register rhs,
                                    Label* lhs_not_nan,
                                    Label* slow,
                                    bool strict);
static void EmitStrictTwoHeapObjectCompare(MacroAssembler* masm,
                                           Register lhs,
                                           Register rhs);


void HydrogenCodeStub::GenerateLightweightMiss(MacroAssembler* masm,
                                               ExternalReference miss) {
  // Update the static counter each time a new code stub is generated.
  isolate()->counters()->code_stubs()->Increment();

  CallInterfaceDescriptor descriptor = GetCallInterfaceDescriptor();
  int param_count = descriptor.GetEnvironmentParameterCount();
  {
    // Call the runtime system in a fresh internal frame.
    FrameAndConstantPoolScope scope(masm, StackFrame::INTERNAL);
    DCHECK(param_count == 0 ||
           r0.is(descriptor.GetEnvironmentParameterRegister(param_count - 1)));
    // Push arguments
    for (int i = 0; i < param_count; ++i) {
      __ push(descriptor.GetEnvironmentParameterRegister(i));
    }
    __ CallExternalReference(miss, param_count);
  }

  __ Ret();
}


void DoubleToIStub::Generate(MacroAssembler* masm) {
  Label out_of_range, only_low, negate, done;
  Register input_reg = source();
  Register result_reg = destination();
  DCHECK(is_truncating());

  int double_offset = offset();
  // Account for saved regs if input is sp.
  if (input_reg.is(sp)) double_offset += 3 * kPointerSize;

  Register scratch = GetRegisterThatIsNotOneOf(input_reg, result_reg);
  Register scratch_low =
      GetRegisterThatIsNotOneOf(input_reg, result_reg, scratch);
  Register scratch_high =
      GetRegisterThatIsNotOneOf(input_reg, result_reg, scratch, scratch_low);
  LowDwVfpRegister double_scratch = kScratchDoubleReg;

  __ Push(scratch_high, scratch_low, scratch);

  if (!skip_fastpath()) {
    // Load double input.
    __ vldr(double_scratch, MemOperand(input_reg, double_offset));
    __ vmov(scratch_low, scratch_high, double_scratch);

    // Do fast-path convert from double to int.
    __ vcvt_s32_f64(double_scratch.low(), double_scratch);
    __ vmov(result_reg, double_scratch.low());

    // If result is not saturated (0x7fffffff or 0x80000000), we are done.
    __ sub(scratch, result_reg, Operand(1));
    __ cmp(scratch, Operand(0x7ffffffe));
    __ b(lt, &done);
  } else {
    // We've already done MacroAssembler::TryFastTruncatedDoubleToILoad, so we
    // know exponent > 31, so we can skip the vcvt_s32_f64 which will saturate.
    if (double_offset == 0) {
      __ ldm(ia, input_reg, scratch_low.bit() | scratch_high.bit());
    } else {
      __ ldr(scratch_low, MemOperand(input_reg, double_offset));
      __ ldr(scratch_high, MemOperand(input_reg, double_offset + kIntSize));
    }
  }

  __ Ubfx(scratch, scratch_high,
         HeapNumber::kExponentShift, HeapNumber::kExponentBits);
  // Load scratch with exponent - 1. This is faster than loading
  // with exponent because Bias + 1 = 1024 which is an *ARM* immediate value.
  STATIC_ASSERT(HeapNumber::kExponentBias + 1 == 1024);
  __ sub(scratch, scratch, Operand(HeapNumber::kExponentBias + 1));
  // If exponent is greater than or equal to 84, the 32 less significant
  // bits are 0s (2^84 = 1, 52 significant bits, 32 uncoded bits),
  // the result is 0.
  // Compare exponent with 84 (compare exponent - 1 with 83).
  __ cmp(scratch, Operand(83));
  __ b(ge, &out_of_range);

  // If we reach this code, 31 <= exponent <= 83.
  // So, we don't have to handle cases where 0 <= exponent <= 20 for
  // which we would need to shift right the high part of the mantissa.
  // Scratch contains exponent - 1.
  // Load scratch with 52 - exponent (load with 51 - (exponent - 1)).
  __ rsb(scratch, scratch, Operand(51), SetCC);
  __ b(ls, &only_low);
  // 21 <= exponent <= 51, shift scratch_low and scratch_high
  // to generate the result.
  __ mov(scratch_low, Operand(scratch_low, LSR, scratch));
  // Scratch contains: 52 - exponent.
  // We needs: exponent - 20.
  // So we use: 32 - scratch = 32 - 52 + exponent = exponent - 20.
  __ rsb(scratch, scratch, Operand(32));
  __ Ubfx(result_reg, scratch_high,
          0, HeapNumber::kMantissaBitsInTopWord);
  // Set the implicit 1 before the mantissa part in scratch_high.
  __ orr(result_reg, result_reg,
         Operand(1 << HeapNumber::kMantissaBitsInTopWord));
  __ orr(result_reg, scratch_low, Operand(result_reg, LSL, scratch));
  __ b(&negate);

  __ bind(&out_of_range);
  __ mov(result_reg, Operand::Zero());
  __ b(&done);

  __ bind(&only_low);
  // 52 <= exponent <= 83, shift only scratch_low.
  // On entry, scratch contains: 52 - exponent.
  __ rsb(scratch, scratch, Operand::Zero());
  __ mov(result_reg, Operand(scratch_low, LSL, scratch));

  __ bind(&negate);
  // If input was positive, scratch_high ASR 31 equals 0 and
  // scratch_high LSR 31 equals zero.
  // New result = (result eor 0) + 0 = result.
  // If the input was negative, we have to negate the result.
  // Input_high ASR 31 equals 0xffffffff and scratch_high LSR 31 equals 1.
  // New result = (result eor 0xffffffff) + 1 = 0 - result.
  __ eor(result_reg, result_reg, Operand(scratch_high, ASR, 31));
  __ add(result_reg, result_reg, Operand(scratch_high, LSR, 31));

  __ bind(&done);

  __ Pop(scratch_high, scratch_low, scratch);
  __ Ret();
}


void WriteInt32ToHeapNumberStub::GenerateFixedRegStubsAheadOfTime(
    Isolate* isolate) {
  WriteInt32ToHeapNumberStub stub1(isolate, r1, r0, r2);
  WriteInt32ToHeapNumberStub stub2(isolate, r2, r0, r3);
  stub1.GetCode();
  stub2.GetCode();
}


// See comment for class.
void WriteInt32ToHeapNumberStub::Generate(MacroAssembler* masm) {
  Label max_negative_int;
  // the_int_ has the answer which is a signed int32 but not a Smi.
  // We test for the special value that has a different exponent.  This test
  // has the neat side effect of setting the flags according to the sign.
  STATIC_ASSERT(HeapNumber::kSignMask == 0x80000000u);
  __ cmp(the_int(), Operand(0x80000000u));
  __ b(eq, &max_negative_int);
  // Set up the correct exponent in scratch_.  All non-Smi int32s have the same.
  // A non-Smi integer is 1.xxx * 2^30 so the exponent is 30 (biased).
  uint32_t non_smi_exponent =
      (HeapNumber::kExponentBias + 30) << HeapNumber::kExponentShift;
  __ mov(scratch(), Operand(non_smi_exponent));
  // Set the sign bit in scratch_ if the value was negative.
  __ orr(scratch(), scratch(), Operand(HeapNumber::kSignMask), LeaveCC, cs);
  // Subtract from 0 if the value was negative.
  __ rsb(the_int(), the_int(), Operand::Zero(), LeaveCC, cs);
  // We should be masking the implict first digit of the mantissa away here,
  // but it just ends up combining harmlessly with the last digit of the
  // exponent that happens to be 1.  The sign bit is 0 so we shift 10 to get
  // the most significant 1 to hit the last bit of the 12 bit sign and exponent.
  DCHECK(((1 << HeapNumber::kExponentShift) & non_smi_exponent) != 0);
  const int shift_distance = HeapNumber::kNonMantissaBitsInTopWord - 2;
  __ orr(scratch(), scratch(), Operand(the_int(), LSR, shift_distance));
  __ str(scratch(),
         FieldMemOperand(the_heap_number(), HeapNumber::kExponentOffset));
  __ mov(scratch(), Operand(the_int(), LSL, 32 - shift_distance));
  __ str(scratch(),
         FieldMemOperand(the_heap_number(), HeapNumber::kMantissaOffset));
  __ Ret();

  __ bind(&max_negative_int);
  // The max negative int32 is stored as a positive number in the mantissa of
  // a double because it uses a sign bit instead of using two's complement.
  // The actual mantissa bits stored are all 0 because the implicit most
  // significant 1 bit is not stored.
  non_smi_exponent += 1 << HeapNumber::kExponentShift;
  __ mov(ip, Operand(HeapNumber::kSignMask | non_smi_exponent));
  __ str(ip, FieldMemOperand(the_heap_number(), HeapNumber::kExponentOffset));
  __ mov(ip, Operand::Zero());
  __ str(ip, FieldMemOperand(the_heap_number(), HeapNumber::kMantissaOffset));
  __ Ret();
}


// Handle the case where the lhs and rhs are the same object.
// Equality is almost reflexive (everything but NaN), so this is a test
// for "identity and not NaN".
static void EmitIdenticalObjectComparison(MacroAssembler* masm,
                                          Label* slow,
                                          Condition cond) {
  Label not_identical;
  Label heap_number, return_equal;
  __ cmp(r0, r1);
  __ b(ne, &not_identical);

  // Test for NaN. Sadly, we can't just compare to Factory::nan_value(),
  // so we do the second best thing - test it ourselves.
  // They are both equal and they are not both Smis so both of them are not
  // Smis.  If it's not a heap number, then return equal.
  if (cond == lt || cond == gt) {
    __ CompareObjectType(r0, r4, r4, FIRST_SPEC_OBJECT_TYPE);
    __ b(ge, slow);
  } else {
    __ CompareObjectType(r0, r4, r4, HEAP_NUMBER_TYPE);
    __ b(eq, &heap_number);
    // Comparing JS objects with <=, >= is complicated.
    if (cond != eq) {
      __ cmp(r4, Operand(FIRST_SPEC_OBJECT_TYPE));
      __ b(ge, slow);
      // Normally here we fall through to return_equal, but undefined is
      // special: (undefined == undefined) == true, but
      // (undefined <= undefined) == false!  See ECMAScript 11.8.5.
      if (cond == le || cond == ge) {
        __ cmp(r4, Operand(ODDBALL_TYPE));
        __ b(ne, &return_equal);
        __ LoadRoot(r2, Heap::kUndefinedValueRootIndex);
        __ cmp(r0, r2);
        __ b(ne, &return_equal);
        if (cond == le) {
          // undefined <= undefined should fail.
          __ mov(r0, Operand(GREATER));
        } else  {
          // undefined >= undefined should fail.
          __ mov(r0, Operand(LESS));
        }
        __ Ret();
      }
    }
  }

  __ bind(&return_equal);
  if (cond == lt) {
    __ mov(r0, Operand(GREATER));  // Things aren't less than themselves.
  } else if (cond == gt) {
    __ mov(r0, Operand(LESS));     // Things aren't greater than themselves.
  } else {
    __ mov(r0, Operand(EQUAL));    // Things are <=, >=, ==, === themselves.
  }
  __ Ret();

  // For less and greater we don't have to check for NaN since the result of
  // x < x is false regardless.  For the others here is some code to check
  // for NaN.
  if (cond != lt && cond != gt) {
    __ bind(&heap_number);
    // It is a heap number, so return non-equal if it's NaN and equal if it's
    // not NaN.

    // The representation of NaN values has all exponent bits (52..62) set,
    // and not all mantissa bits (0..51) clear.
    // Read top bits of double representation (second word of value).
    __ ldr(r2, FieldMemOperand(r0, HeapNumber::kExponentOffset));
    // Test that exponent bits are all set.
    __ Sbfx(r3, r2, HeapNumber::kExponentShift, HeapNumber::kExponentBits);
    // NaNs have all-one exponents so they sign extend to -1.
    __ cmp(r3, Operand(-1));
    __ b(ne, &return_equal);

    // Shift out flag and all exponent bits, retaining only mantissa.
    __ mov(r2, Operand(r2, LSL, HeapNumber::kNonMantissaBitsInTopWord));
    // Or with all low-bits of mantissa.
    __ ldr(r3, FieldMemOperand(r0, HeapNumber::kMantissaOffset));
    __ orr(r0, r3, Operand(r2), SetCC);
    // For equal we already have the right value in r0:  Return zero (equal)
    // if all bits in mantissa are zero (it's an Infinity) and non-zero if
    // not (it's a NaN).  For <= and >= we need to load r0 with the failing
    // value if it's a NaN.
    if (cond != eq) {
      // All-zero means Infinity means equal.
      __ Ret(eq);
      if (cond == le) {
        __ mov(r0, Operand(GREATER));  // NaN <= NaN should fail.
      } else {
        __ mov(r0, Operand(LESS));     // NaN >= NaN should fail.
      }
    }
    __ Ret();
  }
  // No fall through here.

  __ bind(&not_identical);
}


// See comment at call site.
static void EmitSmiNonsmiComparison(MacroAssembler* masm,
                                    Register lhs,
                                    Register rhs,
                                    Label* lhs_not_nan,
                                    Label* slow,
                                    bool strict) {
  DCHECK((lhs.is(r0) && rhs.is(r1)) ||
         (lhs.is(r1) && rhs.is(r0)));

  Label rhs_is_smi;
  __ JumpIfSmi(rhs, &rhs_is_smi);

  // Lhs is a Smi.  Check whether the rhs is a heap number.
  __ CompareObjectType(rhs, r4, r4, HEAP_NUMBER_TYPE);
  if (strict) {
    // If rhs is not a number and lhs is a Smi then strict equality cannot
    // succeed.  Return non-equal
    // If rhs is r0 then there is already a non zero value in it.
    if (!rhs.is(r0)) {
      __ mov(r0, Operand(NOT_EQUAL), LeaveCC, ne);
    }
    __ Ret(ne);
  } else {
    // Smi compared non-strictly with a non-Smi non-heap-number.  Call
    // the runtime.
    __ b(ne, slow);
  }

  // Lhs is a smi, rhs is a number.
  // Convert lhs to a double in d7.
  __ SmiToDouble(d7, lhs);
  // Load the double from rhs, tagged HeapNumber r0, to d6.
  __ vldr(d6, rhs, HeapNumber::kValueOffset - kHeapObjectTag);

  // We now have both loaded as doubles but we can skip the lhs nan check
  // since it's a smi.
  __ jmp(lhs_not_nan);

  __ bind(&rhs_is_smi);
  // Rhs is a smi.  Check whether the non-smi lhs is a heap number.
  __ CompareObjectType(lhs, r4, r4, HEAP_NUMBER_TYPE);
  if (strict) {
    // If lhs is not a number and rhs is a smi then strict equality cannot
    // succeed.  Return non-equal.
    // If lhs is r0 then there is already a non zero value in it.
    if (!lhs.is(r0)) {
      __ mov(r0, Operand(NOT_EQUAL), LeaveCC, ne);
    }
    __ Ret(ne);
  } else {
    // Smi compared non-strictly with a non-smi non-heap-number.  Call
    // the runtime.
    __ b(ne, slow);
  }

  // Rhs is a smi, lhs is a heap number.
  // Load the double from lhs, tagged HeapNumber r1, to d7.
  __ vldr(d7, lhs, HeapNumber::kValueOffset - kHeapObjectTag);
  // Convert rhs to a double in d6              .
  __ SmiToDouble(d6, rhs);
  // Fall through to both_loaded_as_doubles.
}


// See comment at call site.
static void EmitStrictTwoHeapObjectCompare(MacroAssembler* masm,
                                           Register lhs,
                                           Register rhs) {
    DCHECK((lhs.is(r0) && rhs.is(r1)) ||
           (lhs.is(r1) && rhs.is(r0)));

    // If either operand is a JS object or an oddball value, then they are
    // not equal since their pointers are different.
    // There is no test for undetectability in strict equality.
    STATIC_ASSERT(LAST_TYPE == LAST_SPEC_OBJECT_TYPE);
    Label first_non_object;
    // Get the type of the first operand into r2 and compare it with
    // FIRST_SPEC_OBJECT_TYPE.
    __ CompareObjectType(rhs, r2, r2, FIRST_SPEC_OBJECT_TYPE);
    __ b(lt, &first_non_object);

    // Return non-zero (r0 is not zero)
    Label return_not_equal;
    __ bind(&return_not_equal);
    __ Ret();

    __ bind(&first_non_object);
    // Check for oddballs: true, false, null, undefined.
    __ cmp(r2, Operand(ODDBALL_TYPE));
    __ b(eq, &return_not_equal);

    __ CompareObjectType(lhs, r3, r3, FIRST_SPEC_OBJECT_TYPE);
    __ b(ge, &return_not_equal);

    // Check for oddballs: true, false, null, undefined.
    __ cmp(r3, Operand(ODDBALL_TYPE));
    __ b(eq, &return_not_equal);

    // Now that we have the types we might as well check for
    // internalized-internalized.
    STATIC_ASSERT(kInternalizedTag == 0 && kStringTag == 0);
    __ orr(r2, r2, Operand(r3));
    __ tst(r2, Operand(kIsNotStringMask | kIsNotInternalizedMask));
    __ b(eq, &return_not_equal);
}


// See comment at call site.
static void EmitCheckForTwoHeapNumbers(MacroAssembler* masm,
                                       Register lhs,
                                       Register rhs,
                                       Label* both_loaded_as_doubles,
                                       Label* not_heap_numbers,
                                       Label* slow) {
  DCHECK((lhs.is(r0) && rhs.is(r1)) ||
         (lhs.is(r1) && rhs.is(r0)));

  __ CompareObjectType(rhs, r3, r2, HEAP_NUMBER_TYPE);
  __ b(ne, not_heap_numbers);
  __ ldr(r2, FieldMemOperand(lhs, HeapObject::kMapOffset));
  __ cmp(r2, r3);
  __ b(ne, slow);  // First was a heap number, second wasn't.  Go slow case.

  // Both are heap numbers.  Load them up then jump to the code we have
  // for that.
  __ vldr(d6, rhs, HeapNumber::kValueOffset - kHeapObjectTag);
  __ vldr(d7, lhs, HeapNumber::kValueOffset - kHeapObjectTag);
  __ jmp(both_loaded_as_doubles);
}


// Fast negative check for internalized-to-internalized equality.
static void EmitCheckForInternalizedStringsOrObjects(MacroAssembler* masm,
                                                     Register lhs,
                                                     Register rhs,
                                                     Label* possible_strings,
                                                     Label* not_both_strings) {
  DCHECK((lhs.is(r0) && rhs.is(r1)) ||
         (lhs.is(r1) && rhs.is(r0)));

  // r2 is object type of rhs.
  Label object_test;
  STATIC_ASSERT(kInternalizedTag == 0 && kStringTag == 0);
  __ tst(r2, Operand(kIsNotStringMask));
  __ b(ne, &object_test);
  __ tst(r2, Operand(kIsNotInternalizedMask));
  __ b(ne, possible_strings);
  __ CompareObjectType(lhs, r3, r3, FIRST_NONSTRING_TYPE);
  __ b(ge, not_both_strings);
  __ tst(r3, Operand(kIsNotInternalizedMask));
  __ b(ne, possible_strings);

  // Both are internalized.  We already checked they weren't the same pointer
  // so they are not equal.
  __ mov(r0, Operand(NOT_EQUAL));
  __ Ret();

  __ bind(&object_test);
  __ cmp(r2, Operand(FIRST_SPEC_OBJECT_TYPE));
  __ b(lt, not_both_strings);
  __ CompareObjectType(lhs, r2, r3, FIRST_SPEC_OBJECT_TYPE);
  __ b(lt, not_both_strings);
  // If both objects are undetectable, they are equal. Otherwise, they
  // are not equal, since they are different objects and an object is not
  // equal to undefined.
  __ ldr(r3, FieldMemOperand(rhs, HeapObject::kMapOffset));
  __ ldrb(r2, FieldMemOperand(r2, Map::kBitFieldOffset));
  __ ldrb(r3, FieldMemOperand(r3, Map::kBitFieldOffset));
  __ and_(r0, r2, Operand(r3));
  __ and_(r0, r0, Operand(1 << Map::kIsUndetectable));
  __ eor(r0, r0, Operand(1 << Map::kIsUndetectable));
  __ Ret();
}


static void CompareICStub_CheckInputType(MacroAssembler* masm, Register input,
                                         Register scratch,
                                         CompareICState::State expected,
                                         Label* fail) {
  Label ok;
  if (expected == CompareICState::SMI) {
    __ JumpIfNotSmi(input, fail);
  } else if (expected == CompareICState::NUMBER) {
    __ JumpIfSmi(input, &ok);
    __ CheckMap(input, scratch, Heap::kHeapNumberMapRootIndex, fail,
                DONT_DO_SMI_CHECK);
  }
  // We could be strict about internalized/non-internalized here, but as long as
  // hydrogen doesn't care, the stub doesn't have to care either.
  __ bind(&ok);
}


// On entry r1 and r2 are the values to be compared.
// On exit r0 is 0, positive or negative to indicate the result of
// the comparison.
void CompareICStub::GenerateGeneric(MacroAssembler* masm) {
  Register lhs = r1;
  Register rhs = r0;
  Condition cc = GetCondition();

  Label miss;
  CompareICStub_CheckInputType(masm, lhs, r2, left(), &miss);
  CompareICStub_CheckInputType(masm, rhs, r3, right(), &miss);

  Label slow;  // Call builtin.
  Label not_smis, both_loaded_as_doubles, lhs_not_nan;

  Label not_two_smis, smi_done;
  __ orr(r2, r1, r0);
  __ JumpIfNotSmi(r2, &not_two_smis);
  __ mov(r1, Operand(r1, ASR, 1));
  __ sub(r0, r1, Operand(r0, ASR, 1));
  __ Ret();
  __ bind(&not_two_smis);

  // NOTICE! This code is only reached after a smi-fast-case check, so
  // it is certain that at least one operand isn't a smi.

  // Handle the case where the objects are identical.  Either returns the answer
  // or goes to slow.  Only falls through if the objects were not identical.
  EmitIdenticalObjectComparison(masm, &slow, cc);

  // If either is a Smi (we know that not both are), then they can only
  // be strictly equal if the other is a HeapNumber.
  STATIC_ASSERT(kSmiTag == 0);
  DCHECK_EQ(0, Smi::FromInt(0));
  __ and_(r2, lhs, Operand(rhs));
  __ JumpIfNotSmi(r2, &not_smis);
  // One operand is a smi.  EmitSmiNonsmiComparison generates code that can:
  // 1) Return the answer.
  // 2) Go to slow.
  // 3) Fall through to both_loaded_as_doubles.
  // 4) Jump to lhs_not_nan.
  // In cases 3 and 4 we have found out we were dealing with a number-number
  // comparison.  If VFP3 is supported the double values of the numbers have
  // been loaded into d7 and d6.  Otherwise, the double values have been loaded
  // into r0, r1, r2, and r3.
  EmitSmiNonsmiComparison(masm, lhs, rhs, &lhs_not_nan, &slow, strict());

  __ bind(&both_loaded_as_doubles);
  // The arguments have been converted to doubles and stored in d6 and d7, if
  // VFP3 is supported, or in r0, r1, r2, and r3.
  __ bind(&lhs_not_nan);
  Label no_nan;
  // ARMv7 VFP3 instructions to implement double precision comparison.
  __ VFPCompareAndSetFlags(d7, d6);
  Label nan;
  __ b(vs, &nan);
  __ mov(r0, Operand(EQUAL), LeaveCC, eq);
  __ mov(r0, Operand(LESS), LeaveCC, lt);
  __ mov(r0, Operand(GREATER), LeaveCC, gt);
  __ Ret();

  __ bind(&nan);
  // If one of the sides was a NaN then the v flag is set.  Load r0 with
  // whatever it takes to make the comparison fail, since comparisons with NaN
  // always fail.
  if (cc == lt || cc == le) {
    __ mov(r0, Operand(GREATER));
  } else {
    __ mov(r0, Operand(LESS));
  }
  __ Ret();

  __ bind(&not_smis);
  // At this point we know we are dealing with two different objects,
  // and neither of them is a Smi.  The objects are in rhs_ and lhs_.
  if (strict()) {
    // This returns non-equal for some object types, or falls through if it
    // was not lucky.
    EmitStrictTwoHeapObjectCompare(masm, lhs, rhs);
  }

  Label check_for_internalized_strings;
  Label flat_string_check;
  // Check for heap-number-heap-number comparison.  Can jump to slow case,
  // or load both doubles into r0, r1, r2, r3 and jump to the code that handles
  // that case.  If the inputs are not doubles then jumps to
  // check_for_internalized_strings.
  // In this case r2 will contain the type of rhs_.  Never falls through.
  EmitCheckForTwoHeapNumbers(masm,
                             lhs,
                             rhs,
                             &both_loaded_as_doubles,
                             &check_for_internalized_strings,
                             &flat_string_check);

  __ bind(&check_for_internalized_strings);
  // In the strict case the EmitStrictTwoHeapObjectCompare already took care of
  // internalized strings.
  if (cc == eq && !strict()) {
    // Returns an answer for two internalized strings or two detectable objects.
    // Otherwise jumps to string case or not both strings case.
    // Assumes that r2 is the type of rhs_ on entry.
    EmitCheckForInternalizedStringsOrObjects(
        masm, lhs, rhs, &flat_string_check, &slow);
  }

  // Check for both being sequential one-byte strings,
  // and inline if that is the case.
  __ bind(&flat_string_check);

  __ JumpIfNonSmisNotBothSequentialOneByteStrings(lhs, rhs, r2, r3, &slow);

  __ IncrementCounter(isolate()->counters()->string_compare_native(), 1, r2,
                      r3);
  if (cc == eq) {
    StringHelper::GenerateFlatOneByteStringEquals(masm, lhs, rhs, r2, r3, r4);
  } else {
    StringHelper::GenerateCompareFlatOneByteStrings(masm, lhs, rhs, r2, r3, r4,
                                                    r5);
  }
  // Never falls through to here.

  __ bind(&slow);

  __ Push(lhs, rhs);
  // Figure out which native to call and setup the arguments.
  Builtins::JavaScript native;
  if (cc == eq) {
    native = strict() ? Builtins::STRICT_EQUALS : Builtins::EQUALS;
  } else {
    native = Builtins::COMPARE;
    int ncr;  // NaN compare result
    if (cc == lt || cc == le) {
      ncr = GREATER;
    } else {
      DCHECK(cc == gt || cc == ge);  // remaining cases
      ncr = LESS;
    }
    __ mov(r0, Operand(Smi::FromInt(ncr)));
    __ push(r0);
  }

  // Call the native; it returns -1 (less), 0 (equal), or 1 (greater)
  // tagged as a small integer.
  __ InvokeBuiltin(native, JUMP_FUNCTION);

  __ bind(&miss);
  GenerateMiss(masm);
}


void StoreBufferOverflowStub::Generate(MacroAssembler* masm) {
  // We don't allow a GC during a store buffer overflow so there is no need to
  // store the registers in any particular way, but we do have to store and
  // restore them.
  __ stm(db_w, sp, kCallerSaved | lr.bit());

  const Register scratch = r1;

  if (save_doubles()) {
    __ SaveFPRegs(sp, scratch);
  }
  const int argument_count = 1;
  const int fp_argument_count = 0;

  AllowExternalCallThatCantCauseGC scope(masm);
  __ PrepareCallCFunction(argument_count, fp_argument_count, scratch);
  __ mov(r0, Operand(ExternalReference::isolate_address(isolate())));
  __ CallCFunction(
      ExternalReference::store_buffer_overflow_function(isolate()),
      argument_count);
  if (save_doubles()) {
    __ RestoreFPRegs(sp, scratch);
  }
  __ ldm(ia_w, sp, kCallerSaved | pc.bit());  // Also pop pc to get Ret(0).
}


void MathPowStub::Generate(MacroAssembler* masm) {
  const Register base = r1;
  const Register exponent = MathPowTaggedDescriptor::exponent();
  DCHECK(exponent.is(r2));
  const Register heapnumbermap = r5;
  const Register heapnumber = r0;
  const DwVfpRegister double_base = d0;
  const DwVfpRegister double_exponent = d1;
  const DwVfpRegister double_result = d2;
  const DwVfpRegister double_scratch = d3;
  const SwVfpRegister single_scratch = s6;
  const Register scratch = r9;
  const Register scratch2 = r4;

  Label call_runtime, done, int_exponent;
  if (exponent_type() == ON_STACK) {
    Label base_is_smi, unpack_exponent;
    // The exponent and base are supplied as arguments on the stack.
    // This can only happen if the stub is called from non-optimized code.
    // Load input parameters from stack to double registers.
    __ ldr(base, MemOperand(sp, 1 * kPointerSize));
    __ ldr(exponent, MemOperand(sp, 0 * kPointerSize));

    __ LoadRoot(heapnumbermap, Heap::kHeapNumberMapRootIndex);

    __ UntagAndJumpIfSmi(scratch, base, &base_is_smi);
    __ ldr(scratch, FieldMemOperand(base, JSObject::kMapOffset));
    __ cmp(scratch, heapnumbermap);
    __ b(ne, &call_runtime);

    __ vldr(double_base, FieldMemOperand(base, HeapNumber::kValueOffset));
    __ jmp(&unpack_exponent);

    __ bind(&base_is_smi);
    __ vmov(single_scratch, scratch);
    __ vcvt_f64_s32(double_base, single_scratch);
    __ bind(&unpack_exponent);

    __ UntagAndJumpIfSmi(scratch, exponent, &int_exponent);

    __ ldr(scratch, FieldMemOperand(exponent, JSObject::kMapOffset));
    __ cmp(scratch, heapnumbermap);
    __ b(ne, &call_runtime);
    __ vldr(double_exponent,
            FieldMemOperand(exponent, HeapNumber::kValueOffset));
  } else if (exponent_type() == TAGGED) {
    // Base is already in double_base.
    __ UntagAndJumpIfSmi(scratch, exponent, &int_exponent);

    __ vldr(double_exponent,
            FieldMemOperand(exponent, HeapNumber::kValueOffset));
  }

  if (exponent_type() != INTEGER) {
    Label int_exponent_convert;
    // Detect integer exponents stored as double.
    __ vcvt_u32_f64(single_scratch, double_exponent);
    // We do not check for NaN or Infinity here because comparing numbers on
    // ARM correctly distinguishes NaNs.  We end up calling the built-in.
    __ vcvt_f64_u32(double_scratch, single_scratch);
    __ VFPCompareAndSetFlags(double_scratch, double_exponent);
    __ b(eq, &int_exponent_convert);

    if (exponent_type() == ON_STACK) {
      // Detect square root case.  Crankshaft detects constant +/-0.5 at
      // compile time and uses DoMathPowHalf instead.  We then skip this check
      // for non-constant cases of +/-0.5 as these hardly occur.
      Label not_plus_half;

      // Test for 0.5.
      __ vmov(double_scratch, 0.5, scratch);
      __ VFPCompareAndSetFlags(double_exponent, double_scratch);
      __ b(ne, &not_plus_half);

      // Calculates square root of base.  Check for the special case of
      // Math.pow(-Infinity, 0.5) == Infinity (ECMA spec, 15.8.2.13).
      __ vmov(double_scratch, -V8_INFINITY, scratch);
      __ VFPCompareAndSetFlags(double_base, double_scratch);
      __ vneg(double_result, double_scratch, eq);
      __ b(eq, &done);

      // Add +0 to convert -0 to +0.
      __ vadd(double_scratch, double_base, kDoubleRegZero);
      __ vsqrt(double_result, double_scratch);
      __ jmp(&done);

      __ bind(&not_plus_half);
      __ vmov(double_scratch, -0.5, scratch);
      __ VFPCompareAndSetFlags(double_exponent, double_scratch);
      __ b(ne, &call_runtime);

      // Calculates square root of base.  Check for the special case of
      // Math.pow(-Infinity, -0.5) == 0 (ECMA spec, 15.8.2.13).
      __ vmov(double_scratch, -V8_INFINITY, scratch);
      __ VFPCompareAndSetFlags(double_base, double_scratch);
      __ vmov(double_result, kDoubleRegZero, eq);
      __ b(eq, &done);

      // Add +0 to convert -0 to +0.
      __ vadd(double_scratch, double_base, kDoubleRegZero);
      __ vmov(double_result, 1.0, scratch);
      __ vsqrt(double_scratch, double_scratch);
      __ vdiv(double_result, double_result, double_scratch);
      __ jmp(&done);
    }

    __ push(lr);
    {
      AllowExternalCallThatCantCauseGC scope(masm);
      __ PrepareCallCFunction(0, 2, scratch);
      __ MovToFloatParameters(double_base, double_exponent);
      __ CallCFunction(
          ExternalReference::power_double_double_function(isolate()),
          0, 2);
    }
    __ pop(lr);
    __ MovFromFloatResult(double_result);
    __ jmp(&done);

    __ bind(&int_exponent_convert);
    __ vcvt_u32_f64(single_scratch, double_exponent);
    __ vmov(scratch, single_scratch);
  }

  // Calculate power with integer exponent.
  __ bind(&int_exponent);

  // Get two copies of exponent in the registers scratch and exponent.
  if (exponent_type() == INTEGER) {
    __ mov(scratch, exponent);
  } else {
    // Exponent has previously been stored into scratch as untagged integer.
    __ mov(exponent, scratch);
  }
  __ vmov(double_scratch, double_base);  // Back up base.
  __ vmov(double_result, 1.0, scratch2);

  // Get absolute value of exponent.
  __ cmp(scratch, Operand::Zero());
  __ mov(scratch2, Operand::Zero(), LeaveCC, mi);
  __ sub(scratch, scratch2, scratch, LeaveCC, mi);

  Label while_true;
  __ bind(&while_true);
  __ mov(scratch, Operand(scratch, ASR, 1), SetCC);
  __ vmul(double_result, double_result, double_scratch, cs);
  __ vmul(double_scratch, double_scratch, double_scratch, ne);
  __ b(ne, &while_true);

  __ cmp(exponent, Operand::Zero());
  __ b(ge, &done);
  __ vmov(double_scratch, 1.0, scratch);
  __ vdiv(double_result, double_scratch, double_result);
  // Test whether result is zero.  Bail out to check for subnormal result.
  // Due to subnormals, x^-y == (1/x)^y does not hold in all cases.
  __ VFPCompareAndSetFlags(double_result, 0.0);
  __ b(ne, &done);
  // double_exponent may not containe the exponent value if the input was a
  // smi.  We set it with exponent value before bailing out.
  __ vmov(single_scratch, exponent);
  __ vcvt_f64_s32(double_exponent, single_scratch);

  // Returning or bailing out.
  Counters* counters = isolate()->counters();
  if (exponent_type() == ON_STACK) {
    // The arguments are still on the stack.
    __ bind(&call_runtime);
    __ TailCallRuntime(Runtime::kMathPowRT, 2, 1);

    // The stub is called from non-optimized code, which expects the result
    // as heap number in exponent.
    __ bind(&done);
    __ AllocateHeapNumber(
        heapnumber, scratch, scratch2, heapnumbermap, &call_runtime);
    __ vstr(double_result,
            FieldMemOperand(heapnumber, HeapNumber::kValueOffset));
    DCHECK(heapnumber.is(r0));
    __ IncrementCounter(counters->math_pow(), 1, scratch, scratch2);
    __ Ret(2);
  } else {
    __ push(lr);
    {
      AllowExternalCallThatCantCauseGC scope(masm);
      __ PrepareCallCFunction(0, 2, scratch);
      __ MovToFloatParameters(double_base, double_exponent);
      __ CallCFunction(
          ExternalReference::power_double_double_function(isolate()),
          0, 2);
    }
    __ pop(lr);
    __ MovFromFloatResult(double_result);

    __ bind(&done);
    __ IncrementCounter(counters->math_pow(), 1, scratch, scratch2);
    __ Ret();
  }
}


bool CEntryStub::NeedsImmovableCode() {
  return true;
}


void CodeStub::GenerateStubsAheadOfTime(Isolate* isolate) {
  CEntryStub::GenerateAheadOfTime(isolate);
  WriteInt32ToHeapNumberStub::GenerateFixedRegStubsAheadOfTime(isolate);
  StoreBufferOverflowStub::GenerateFixedRegStubsAheadOfTime(isolate);
  StubFailureTrampolineStub::GenerateAheadOfTime(isolate);
  ArrayConstructorStubBase::GenerateStubsAheadOfTime(isolate);
  CreateAllocationSiteStub::GenerateAheadOfTime(isolate);
  BinaryOpICStub::GenerateAheadOfTime(isolate);
  BinaryOpICWithAllocationSiteStub::GenerateAheadOfTime(isolate);
}


void CodeStub::GenerateFPStubs(Isolate* isolate) {
  // Generate if not already in cache.
  SaveFPRegsMode mode = kSaveFPRegs;
  CEntryStub(isolate, 1, mode).GetCode();
  StoreBufferOverflowStub(isolate, mode).GetCode();
  isolate->set_fp_stubs_generated(true);
}


void CEntryStub::GenerateAheadOfTime(Isolate* isolate) {
  CEntryStub stub(isolate, 1, kDontSaveFPRegs);
  stub.GetCode();
}


void CEntryStub::Generate(MacroAssembler* masm) {
  // Called from JavaScript; parameters are on stack as if calling JS function.
  // r0: number of arguments including receiver
  // r1: pointer to builtin function
  // fp: frame pointer  (restored after C call)
  // sp: stack pointer  (restored as callee's sp after C call)
  // cp: current context  (C callee-saved)

  ProfileEntryHookStub::MaybeCallEntryHook(masm);

  __ mov(r5, Operand(r1));

  // Compute the argv pointer in a callee-saved register.
  __ add(r1, sp, Operand(r0, LSL, kPointerSizeLog2));
  __ sub(r1, r1, Operand(kPointerSize));

  // Enter the exit frame that transitions from JavaScript to C++.
  FrameScope scope(masm, StackFrame::MANUAL);
  __ EnterExitFrame(save_doubles());

  // Store a copy of argc in callee-saved registers for later.
  __ mov(r4, Operand(r0));

  // r0, r4: number of arguments including receiver  (C callee-saved)
  // r1: pointer to the first argument (C callee-saved)
  // r5: pointer to builtin function  (C callee-saved)

  // Result returned in r0 or r0+r1 by default.

#if V8_HOST_ARCH_ARM
  int frame_alignment = MacroAssembler::ActivationFrameAlignment();
  int frame_alignment_mask = frame_alignment - 1;
  if (FLAG_debug_code) {
    if (frame_alignment > kPointerSize) {
      Label alignment_as_expected;
      DCHECK(base::bits::IsPowerOfTwo32(frame_alignment));
      __ tst(sp, Operand(frame_alignment_mask));
      __ b(eq, &alignment_as_expected);
      // Don't use Check here, as it will call Runtime_Abort re-entering here.
      __ stop("Unexpected alignment");
      __ bind(&alignment_as_expected);
    }
  }
#endif

  // Call C built-in.
  // r0 = argc, r1 = argv
  __ mov(r2, Operand(ExternalReference::isolate_address(isolate())));

  // To let the GC traverse the return address of the exit frames, we need to
  // know where the return address is. The CEntryStub is unmovable, so
  // we can store the address on the stack to be able to find it again and
  // we never have to restore it, because it will not change.
  // Compute the return address in lr to return to after the jump below. Pc is
  // already at '+ 8' from the current instruction but return is after three
  // instructions so add another 4 to pc to get the return address.
  {
    // Prevent literal pool emission before return address.
    Assembler::BlockConstPoolScope block_const_pool(masm);
    __ add(lr, pc, Operand(4));
    __ str(lr, MemOperand(sp, 0));
    __ Call(r5);
  }

  __ VFPEnsureFPSCRState(r2);

  // Runtime functions should not return 'the hole'.  Allowing it to escape may
  // lead to crashes in the IC code later.
  if (FLAG_debug_code) {
    Label okay;
    __ CompareRoot(r0, Heap::kTheHoleValueRootIndex);
    __ b(ne, &okay);
    __ stop("The hole escaped");
    __ bind(&okay);
  }

  // Check result for exception sentinel.
  Label exception_returned;
  __ CompareRoot(r0, Heap::kExceptionRootIndex);
  __ b(eq, &exception_returned);

  ExternalReference pending_exception_address(
      Isolate::kPendingExceptionAddress, isolate());

  // Check that there is no pending exception, otherwise we
  // should have returned the exception sentinel.
  if (FLAG_debug_code) {
    Label okay;
    __ mov(r2, Operand(pending_exception_address));
    __ ldr(r2, MemOperand(r2));
    __ CompareRoot(r2, Heap::kTheHoleValueRootIndex);
    // Cannot use check here as it attempts to generate call into runtime.
    __ b(eq, &okay);
    __ stop("Unexpected pending exception");
    __ bind(&okay);
  }

  // Exit C frame and return.
  // r0:r1: result
  // sp: stack pointer
  // fp: frame pointer
  // Callee-saved register r4 still holds argc.
  __ LeaveExitFrame(save_doubles(), r4, true);
  __ mov(pc, lr);

  // Handling of exception.
  __ bind(&exception_returned);

  // Retrieve the pending exception.
  __ mov(r2, Operand(pending_exception_address));
  __ ldr(r0, MemOperand(r2));

  // Clear the pending exception.
  __ LoadRoot(r3, Heap::kTheHoleValueRootIndex);
  __ str(r3, MemOperand(r2));

  // Special handling of termination exceptions which are uncatchable
  // by javascript code.
  Label throw_termination_exception;
  __ CompareRoot(r0, Heap::kTerminationExceptionRootIndex);
  __ b(eq, &throw_termination_exception);

  // Handle normal exception.
  __ Throw(r0);

  __ bind(&throw_termination_exception);
  __ ThrowUncatchable(r0);
}


void JSEntryStub::Generate(MacroAssembler* masm) {
  // r0: code entry
  // r1: function
  // r2: receiver
  // r3: argc
  // [sp+0]: argv

  Label invoke, handler_entry, exit;

  ProfileEntryHookStub::MaybeCallEntryHook(masm);

  // Called from C, so do not pop argc and args on exit (preserve sp)
  // No need to save register-passed args
  // Save callee-saved registers (incl. cp and fp), sp, and lr
  __ stm(db_w, sp, kCalleeSaved | lr.bit());

  // Save callee-saved vfp registers.
  __ vstm(db_w, sp, kFirstCalleeSavedDoubleReg, kLastCalleeSavedDoubleReg);
  // Set up the reserved register for 0.0.
  __ vmov(kDoubleRegZero, 0.0);
  __ VFPEnsureFPSCRState(r4);

  // Get address of argv, see stm above.
  // r0: code entry
  // r1: function
  // r2: receiver
  // r3: argc

  // Set up argv in r4.
  int offset_to_argv = (kNumCalleeSaved + 1) * kPointerSize;
  offset_to_argv += kNumDoubleCalleeSaved * kDoubleSize;
  __ ldr(r4, MemOperand(sp, offset_to_argv));

  // Push a frame with special values setup to mark it as an entry frame.
  // r0: code entry
  // r1: function
  // r2: receiver
  // r3: argc
  // r4: argv
  int marker = type();
  if (FLAG_enable_ool_constant_pool) {
    __ mov(r8, Operand(isolate()->factory()->empty_constant_pool_array()));
  }
  __ mov(r7, Operand(Smi::FromInt(marker)));
  __ mov(r6, Operand(Smi::FromInt(marker)));
  __ mov(r5,
         Operand(ExternalReference(Isolate::kCEntryFPAddress, isolate())));
  __ ldr(r5, MemOperand(r5));
  __ mov(ip, Operand(-1));  // Push a bad frame pointer to fail if it is used.
  __ stm(db_w, sp, r5.bit() | r6.bit() | r7.bit() |
                   (FLAG_enable_ool_constant_pool ? r8.bit() : 0) |
                   ip.bit());

  // Set up frame pointer for the frame to be pushed.
  __ add(fp, sp, Operand(-EntryFrameConstants::kCallerFPOffset));

  // If this is the outermost JS call, set js_entry_sp value.
  Label non_outermost_js;
  ExternalReference js_entry_sp(Isolate::kJSEntrySPAddress, isolate());
  __ mov(r5, Operand(ExternalReference(js_entry_sp)));
  __ ldr(r6, MemOperand(r5));
  __ cmp(r6, Operand::Zero());
  __ b(ne, &non_outermost_js);
  __ str(fp, MemOperand(r5));
  __ mov(ip, Operand(Smi::FromInt(StackFrame::OUTERMOST_JSENTRY_FRAME)));
  Label cont;
  __ b(&cont);
  __ bind(&non_outermost_js);
  __ mov(ip, Operand(Smi::FromInt(StackFrame::INNER_JSENTRY_FRAME)));
  __ bind(&cont);
  __ push(ip);

  // Jump to a faked try block that does the invoke, with a faked catch
  // block that sets the pending exception.
  __ jmp(&invoke);

  // Block literal pool emission whilst taking the position of the handler
  // entry. This avoids making the assumption that literal pools are always
  // emitted after an instruction is emitted, rather than before.
  {
    Assembler::BlockConstPoolScope block_const_pool(masm);
    __ bind(&handler_entry);
    handler_offset_ = handler_entry.pos();
    // Caught exception: Store result (exception) in the pending exception
    // field in the JSEnv and return a failure sentinel.  Coming in here the
    // fp will be invalid because the PushTryHandler below sets it to 0 to
    // signal the existence of the JSEntry frame.
    __ mov(ip, Operand(ExternalReference(Isolate::kPendingExceptionAddress,
                                         isolate())));
  }
  __ str(r0, MemOperand(ip));
  __ LoadRoot(r0, Heap::kExceptionRootIndex);
  __ b(&exit);

  // Invoke: Link this frame into the handler chain.  There's only one
  // handler block in this code object, so its index is 0.
  __ bind(&invoke);
  // Must preserve r0-r4, r5-r6 are available.
  __ PushTryHandler(StackHandler::JS_ENTRY, 0);
  // If an exception not caught by another handler occurs, this handler
  // returns control to the code after the bl(&invoke) above, which
  // restores all kCalleeSaved registers (including cp and fp) to their
  // saved values before returning a failure to C.

  // Clear any pending exceptions.
  __ mov(r5, Operand(isolate()->factory()->the_hole_value()));
  __ mov(ip, Operand(ExternalReference(Isolate::kPendingExceptionAddress,
                                       isolate())));
  __ str(r5, MemOperand(ip));

  // Invoke the function by calling through JS entry trampoline builtin.
  // Notice that we cannot store a reference to the trampoline code directly in
  // this stub, because runtime stubs are not traversed when doing GC.

  // Expected registers by Builtins::JSEntryTrampoline
  // r0: code entry
  // r1: function
  // r2: receiver
  // r3: argc
  // r4: argv
  if (type() == StackFrame::ENTRY_CONSTRUCT) {
    ExternalReference construct_entry(Builtins::kJSConstructEntryTrampoline,
                                      isolate());
    __ mov(ip, Operand(construct_entry));
  } else {
    ExternalReference entry(Builtins::kJSEntryTrampoline, isolate());
    __ mov(ip, Operand(entry));
  }
  __ ldr(ip, MemOperand(ip));  // deref address
  __ add(ip, ip, Operand(Code::kHeaderSize - kHeapObjectTag));

  // Branch and link to JSEntryTrampoline.
  __ Call(ip);

  // Unlink this frame from the handler chain.
  __ PopTryHandler();

  __ bind(&exit);  // r0 holds result
  // Check if the current stack frame is marked as the outermost JS frame.
  Label non_outermost_js_2;
  __ pop(r5);
  __ cmp(r5, Operand(Smi::FromInt(StackFrame::OUTERMOST_JSENTRY_FRAME)));
  __ b(ne, &non_outermost_js_2);
  __ mov(r6, Operand::Zero());
  __ mov(r5, Operand(ExternalReference(js_entry_sp)));
  __ str(r6, MemOperand(r5));
  __ bind(&non_outermost_js_2);

  // Restore the top frame descriptors from the stack.
  __ pop(r3);
  __ mov(ip,
         Operand(ExternalReference(Isolate::kCEntryFPAddress, isolate())));
  __ str(r3, MemOperand(ip));

  // Reset the stack to the callee saved registers.
  __ add(sp, sp, Operand(-EntryFrameConstants::kCallerFPOffset));

  // Restore callee-saved registers and return.
#ifdef DEBUG
  if (FLAG_debug_code) {
    __ mov(lr, Operand(pc));
  }
#endif

  // Restore callee-saved vfp registers.
  __ vldm(ia_w, sp, kFirstCalleeSavedDoubleReg, kLastCalleeSavedDoubleReg);

  __ ldm(ia_w, sp, kCalleeSaved | pc.bit());
}


// Uses registers r0 to r4.
// Expected input (depending on whether args are in registers or on the stack):
// * object: r0 or at sp + 1 * kPointerSize.
// * function: r1 or at sp.
//
// An inlined call site may have been generated before calling this stub.
// In this case the offset to the inline sites to patch are passed in r5 and r6.
// (See LCodeGen::DoInstanceOfKnownGlobal)
void InstanceofStub::Generate(MacroAssembler* masm) {
  // Call site inlining and patching implies arguments in registers.
  DCHECK(HasArgsInRegisters() || !HasCallSiteInlineCheck());

  // Fixed register usage throughout the stub:
  const Register object = r0;  // Object (lhs).
  Register map = r3;  // Map of the object.
  const Register function = r1;  // Function (rhs).
  const Register prototype = r4;  // Prototype of the function.
  const Register scratch = r2;

  Label slow, loop, is_instance, is_not_instance, not_js_object;

  if (!HasArgsInRegisters()) {
    __ ldr(object, MemOperand(sp, 1 * kPointerSize));
    __ ldr(function, MemOperand(sp, 0));
  }

  // Check that the left hand is a JS object and load map.
  __ JumpIfSmi(object, &not_js_object);
  __ IsObjectJSObjectType(object, map, scratch, &not_js_object);

  // If there is a call site cache don't look in the global cache, but do the
  // real lookup and update the call site cache.
  if (!HasCallSiteInlineCheck() && !ReturnTrueFalseObject()) {
    Label miss;
    __ CompareRoot(function, Heap::kInstanceofCacheFunctionRootIndex);
    __ b(ne, &miss);
    __ CompareRoot(map, Heap::kInstanceofCacheMapRootIndex);
    __ b(ne, &miss);
    __ LoadRoot(r0, Heap::kInstanceofCacheAnswerRootIndex);
    __ Ret(HasArgsInRegisters() ? 0 : 2);

    __ bind(&miss);
  }

  // Get the prototype of the function.
  __ TryGetFunctionPrototype(function, prototype, scratch, &slow, true);

  // Check that the function prototype is a JS object.
  __ JumpIfSmi(prototype, &slow);
  __ IsObjectJSObjectType(prototype, scratch, scratch, &slow);

  // Update the global instanceof or call site inlined cache with the current
  // map and function. The cached answer will be set when it is known below.
  if (!HasCallSiteInlineCheck()) {
    __ StoreRoot(function, Heap::kInstanceofCacheFunctionRootIndex);
    __ StoreRoot(map, Heap::kInstanceofCacheMapRootIndex);
  } else {
    DCHECK(HasArgsInRegisters());
    // Patch the (relocated) inlined map check.

    // The map_load_offset was stored in r5
    //   (See LCodeGen::DoDeferredLInstanceOfKnownGlobal).
    const Register map_load_offset = r5;
    __ sub(r9, lr, map_load_offset);
    // Get the map location in r5 and patch it.
    __ GetRelocatedValueLocation(r9, map_load_offset, scratch);
    __ ldr(map_load_offset, MemOperand(map_load_offset));
    __ str(map, FieldMemOperand(map_load_offset, Cell::kValueOffset));
  }

  // Register mapping: r3 is object map and r4 is function prototype.
  // Get prototype of object into r2.
  __ ldr(scratch, FieldMemOperand(map, Map::kPrototypeOffset));

  // We don't need map any more. Use it as a scratch register.
  Register scratch2 = map;
  map = no_reg;

  // Loop through the prototype chain looking for the function prototype.
  __ LoadRoot(scratch2, Heap::kNullValueRootIndex);
  __ bind(&loop);
  __ cmp(scratch, Operand(prototype));
  __ b(eq, &is_instance);
  __ cmp(scratch, scratch2);
  __ b(eq, &is_not_instance);
  __ ldr(scratch, FieldMemOperand(scratch, HeapObject::kMapOffset));
  __ ldr(scratch, FieldMemOperand(scratch, Map::kPrototypeOffset));
  __ jmp(&loop);
  Factory* factory = isolate()->factory();

  __ bind(&is_instance);
  if (!HasCallSiteInlineCheck()) {
    __ mov(r0, Operand(Smi::FromInt(0)));
    __ StoreRoot(r0, Heap::kInstanceofCacheAnswerRootIndex);
    if (ReturnTrueFalseObject()) {
      __ Move(r0, factory->true_value());
    }
  } else {
    // Patch the call site to return true.
    __ LoadRoot(r0, Heap::kTrueValueRootIndex);
    // The bool_load_offset was stored in r6
    //   (See LCodeGen::DoDeferredLInstanceOfKnownGlobal).
    const Register bool_load_offset = r6;
    __ sub(r9, lr, bool_load_offset);
    // Get the boolean result location in scratch and patch it.
    __ GetRelocatedValueLocation(r9, scratch, scratch2);
    __ str(r0, MemOperand(scratch));

    if (!ReturnTrueFalseObject()) {
      __ mov(r0, Operand(Smi::FromInt(0)));
    }
  }
  __ Ret(HasArgsInRegisters() ? 0 : 2);

  __ bind(&is_not_instance);
  if (!HasCallSiteInlineCheck()) {
    __ mov(r0, Operand(Smi::FromInt(1)));
    __ StoreRoot(r0, Heap::kInstanceofCacheAnswerRootIndex);
    if (ReturnTrueFalseObject()) {
      __ Move(r0, factory->false_value());
    }
  } else {
    // Patch the call site to return false.
    __ LoadRoot(r0, Heap::kFalseValueRootIndex);
    // The bool_load_offset was stored in r6
    //   (See LCodeGen::DoDeferredLInstanceOfKnownGlobal).
    const Register bool_load_offset = r6;
    __ sub(r9, lr, bool_load_offset);
    ;
    // Get the boolean result location in scratch and patch it.
    __ GetRelocatedValueLocation(r9, scratch, scratch2);
    __ str(r0, MemOperand(scratch));

    if (!ReturnTrueFalseObject()) {
      __ mov(r0, Operand(Smi::FromInt(1)));
    }
  }
  __ Ret(HasArgsInRegisters() ? 0 : 2);

  Label object_not_null, object_not_null_or_smi;
  __ bind(&not_js_object);
  // Before null, smi and string value checks, check that the rhs is a function
  // as for a non-function rhs an exception needs to be thrown.
  __ JumpIfSmi(function, &slow);
  __ CompareObjectType(function, scratch2, scratch, JS_FUNCTION_TYPE);
  __ b(ne, &slow);

  // Null is not instance of anything.
  __ cmp(object, Operand(isolate()->factory()->null_value()));
  __ b(ne, &object_not_null);
  if (ReturnTrueFalseObject()) {
    __ Move(r0, factory->false_value());
  } else {
    __ mov(r0, Operand(Smi::FromInt(1)));
  }
  __ Ret(HasArgsInRegisters() ? 0 : 2);

  __ bind(&object_not_null);
  // Smi values are not instances of anything.
  __ JumpIfNotSmi(object, &object_not_null_or_smi);
  if (ReturnTrueFalseObject()) {
    __ Move(r0, factory->false_value());
  } else {
    __ mov(r0, Operand(Smi::FromInt(1)));
  }
  __ Ret(HasArgsInRegisters() ? 0 : 2);

  __ bind(&object_not_null_or_smi);
  // String values are not instances of anything.
  __ IsObjectJSStringType(object, scratch, &slow);
  if (ReturnTrueFalseObject()) {
    __ Move(r0, factory->false_value());
  } else {
    __ mov(r0, Operand(Smi::FromInt(1)));
  }
  __ Ret(HasArgsInRegisters() ? 0 : 2);

  // Slow-case.  Tail call builtin.
  __ bind(&slow);
  if (!ReturnTrueFalseObject()) {
    if (HasArgsInRegisters()) {
      __ Push(r0, r1);
    }
  __ InvokeBuiltin(Builtins::INSTANCE_OF, JUMP_FUNCTION);
  } else {
    {
      FrameAndConstantPoolScope scope(masm, StackFrame::INTERNAL);
      __ Push(r0, r1);
      __ InvokeBuiltin(Builtins::INSTANCE_OF, CALL_FUNCTION);
    }
    __ cmp(r0, Operand::Zero());
    __ LoadRoot(r0, Heap::kTrueValueRootIndex, eq);
    __ LoadRoot(r0, Heap::kFalseValueRootIndex, ne);
    __ Ret(HasArgsInRegisters() ? 0 : 2);
  }
}


void FunctionPrototypeStub::Generate(MacroAssembler* masm) {
  Label miss;
  Register receiver = LoadDescriptor::ReceiverRegister();

  NamedLoadHandlerCompiler::GenerateLoadFunctionPrototype(masm, receiver, r3,
                                                          r4, &miss);
  __ bind(&miss);
  PropertyAccessCompiler::TailCallBuiltin(
      masm, PropertyAccessCompiler::MissBuiltin(Code::LOAD_IC));
}


void LoadIndexedStringStub::Generate(MacroAssembler* masm) {
  // Return address is in lr.
  Label miss;

  Register receiver = LoadDescriptor::ReceiverRegister();
  Register index = LoadDescriptor::NameRegister();
  Register scratch = r3;
  Register result = r0;
  DCHECK(!scratch.is(receiver) && !scratch.is(index));

  StringCharAtGenerator char_at_generator(receiver, index, scratch, result,
                                          &miss,  // When not a string.
                                          &miss,  // When not a number.
                                          &miss,  // When index out of range.
                                          STRING_INDEX_IS_ARRAY_INDEX,
                                          RECEIVER_IS_STRING);
  char_at_generator.GenerateFast(masm);
  __ Ret();

  StubRuntimeCallHelper call_helper;
  char_at_generator.GenerateSlow(masm, call_helper);

  __ bind(&miss);
  PropertyAccessCompiler::TailCallBuiltin(
      masm, PropertyAccessCompiler::MissBuiltin(Code::KEYED_LOAD_IC));
}


void ArgumentsAccessStub::GenerateReadElement(MacroAssembler* masm) {
  // The displacement is the offset of the last parameter (if any)
  // relative to the frame pointer.
  const int kDisplacement =
      StandardFrameConstants::kCallerSPOffset - kPointerSize;
  DCHECK(r1.is(ArgumentsAccessReadDescriptor::index()));
  DCHECK(r0.is(ArgumentsAccessReadDescriptor::parameter_count()));

  // Check that the key is a smi.
  Label slow;
  __ JumpIfNotSmi(r1, &slow);

  // Check if the calling frame is an arguments adaptor frame.
  Label adaptor;
  __ ldr(r2, MemOperand(fp, StandardFrameConstants::kCallerFPOffset));
  __ ldr(r3, MemOperand(r2, StandardFrameConstants::kContextOffset));
  __ cmp(r3, Operand(Smi::FromInt(StackFrame::ARGUMENTS_ADAPTOR)));
  __ b(eq, &adaptor);

  // Check index against formal parameters count limit passed in
  // through register r0. Use unsigned comparison to get negative
  // check for free.
  __ cmp(r1, r0);
  __ b(hs, &slow);

  // Read the argument from the stack and return it.
  __ sub(r3, r0, r1);
  __ add(r3, fp, Operand::PointerOffsetFromSmiKey(r3));
  __ ldr(r0, MemOperand(r3, kDisplacement));
  __ Jump(lr);

  // Arguments adaptor case: Check index against actual arguments
  // limit found in the arguments adaptor frame. Use unsigned
  // comparison to get negative check for free.
  __ bind(&adaptor);
  __ ldr(r0, MemOperand(r2, ArgumentsAdaptorFrameConstants::kLengthOffset));
  __ cmp(r1, r0);
  __ b(cs, &slow);

  // Read the argument from the adaptor frame and return it.
  __ sub(r3, r0, r1);
  __ add(r3, r2, Operand::PointerOffsetFromSmiKey(r3));
  __ ldr(r0, MemOperand(r3, kDisplacement));
  __ Jump(lr);

  // Slow-case: Handle non-smi or out-of-bounds access to arguments
  // by calling the runtime system.
  __ bind(&slow);
  __ push(r1);
  __ TailCallRuntime(Runtime::kGetArgumentsProperty, 1, 1);
}


void ArgumentsAccessStub::GenerateNewSloppySlow(MacroAssembler* masm) {
  // sp[0] : number of parameters
  // sp[4] : receiver displacement
  // sp[8] : function

  // Check if the calling frame is an arguments adaptor frame.
  Label runtime;
  __ ldr(r3, MemOperand(fp, StandardFrameConstants::kCallerFPOffset));
  __ ldr(r2, MemOperand(r3, StandardFrameConstants::kContextOffset));
  __ cmp(r2, Operand(Smi::FromInt(StackFrame::ARGUMENTS_ADAPTOR)));
  __ b(ne, &runtime);

  // Patch the arguments.length and the parameters pointer in the current frame.
  __ ldr(r2, MemOperand(r3, ArgumentsAdaptorFrameConstants::kLengthOffset));
  __ str(r2, MemOperand(sp, 0 * kPointerSize));
  __ add(r3, r3, Operand(r2, LSL, 1));
  __ add(r3, r3, Operand(StandardFrameConstants::kCallerSPOffset));
  __ str(r3, MemOperand(sp, 1 * kPointerSize));

  __ bind(&runtime);
  __ TailCallRuntime(Runtime::kNewSloppyArguments, 3, 1);
}


void ArgumentsAccessStub::GenerateNewSloppyFast(MacroAssembler* masm) {
  // Stack layout:
  //  sp[0] : number of parameters (tagged)
  //  sp[4] : address of receiver argument
  //  sp[8] : function
  // Registers used over whole function:
  //  r6 : allocated object (tagged)
  //  r9 : mapped parameter count (tagged)

  __ ldr(r1, MemOperand(sp, 0 * kPointerSize));
  // r1 = parameter count (tagged)

  // Check if the calling frame is an arguments adaptor frame.
  Label runtime;
  Label adaptor_frame, try_allocate;
  __ ldr(r3, MemOperand(fp, StandardFrameConstants::kCallerFPOffset));
  __ ldr(r2, MemOperand(r3, StandardFrameConstants::kContextOffset));
  __ cmp(r2, Operand(Smi::FromInt(StackFrame::ARGUMENTS_ADAPTOR)));
  __ b(eq, &adaptor_frame);

  // No adaptor, parameter count = argument count.
  __ mov(r2, r1);
  __ b(&try_allocate);

  // We have an adaptor frame. Patch the parameters pointer.
  __ bind(&adaptor_frame);
  __ ldr(r2, MemOperand(r3, ArgumentsAdaptorFrameConstants::kLengthOffset));
  __ add(r3, r3, Operand(r2, LSL, 1));
  __ add(r3, r3, Operand(StandardFrameConstants::kCallerSPOffset));
  __ str(r3, MemOperand(sp, 1 * kPointerSize));

  // r1 = parameter count (tagged)
  // r2 = argument count (tagged)
  // Compute the mapped parameter count = min(r1, r2) in r1.
  __ cmp(r1, Operand(r2));
  __ mov(r1, Operand(r2), LeaveCC, gt);

  __ bind(&try_allocate);

  // Compute the sizes of backing store, parameter map, and arguments object.
  // 1. Parameter map, has 2 extra words containing context and backing store.
  const int kParameterMapHeaderSize =
      FixedArray::kHeaderSize + 2 * kPointerSize;
  // If there are no mapped parameters, we do not need the parameter_map.
  __ cmp(r1, Operand(Smi::FromInt(0)));
  __ mov(r9, Operand::Zero(), LeaveCC, eq);
  __ mov(r9, Operand(r1, LSL, 1), LeaveCC, ne);
  __ add(r9, r9, Operand(kParameterMapHeaderSize), LeaveCC, ne);

  // 2. Backing store.
  __ add(r9, r9, Operand(r2, LSL, 1));
  __ add(r9, r9, Operand(FixedArray::kHeaderSize));

  // 3. Arguments object.
  __ add(r9, r9, Operand(Heap::kSloppyArgumentsObjectSize));

  // Do the allocation of all three objects in one go.
  __ Allocate(r9, r0, r3, r4, &runtime, TAG_OBJECT);

  // r0 = address of new object(s) (tagged)
  // r2 = argument count (smi-tagged)
  // Get the arguments boilerplate from the current native context into r4.
  const int kNormalOffset =
      Context::SlotOffset(Context::SLOPPY_ARGUMENTS_MAP_INDEX);
  const int kAliasedOffset =
      Context::SlotOffset(Context::ALIASED_ARGUMENTS_MAP_INDEX);

  __ ldr(r4, MemOperand(cp, Context::SlotOffset(Context::GLOBAL_OBJECT_INDEX)));
  __ ldr(r4, FieldMemOperand(r4, GlobalObject::kNativeContextOffset));
  __ cmp(r1, Operand::Zero());
  __ ldr(r4, MemOperand(r4, kNormalOffset), eq);
  __ ldr(r4, MemOperand(r4, kAliasedOffset), ne);

  // r0 = address of new object (tagged)
  // r1 = mapped parameter count (tagged)
  // r2 = argument count (smi-tagged)
  // r4 = address of arguments map (tagged)
  __ str(r4, FieldMemOperand(r0, JSObject::kMapOffset));
  __ LoadRoot(r3, Heap::kEmptyFixedArrayRootIndex);
  __ str(r3, FieldMemOperand(r0, JSObject::kPropertiesOffset));
  __ str(r3, FieldMemOperand(r0, JSObject::kElementsOffset));

  // Set up the callee in-object property.
  STATIC_ASSERT(Heap::kArgumentsCalleeIndex == 1);
  __ ldr(r3, MemOperand(sp, 2 * kPointerSize));
  __ AssertNotSmi(r3);
  const int kCalleeOffset = JSObject::kHeaderSize +
      Heap::kArgumentsCalleeIndex * kPointerSize;
  __ str(r3, FieldMemOperand(r0, kCalleeOffset));

  // Use the length (smi tagged) and set that as an in-object property too.
  __ AssertSmi(r2);
  STATIC_ASSERT(Heap::kArgumentsLengthIndex == 0);
  const int kLengthOffset = JSObject::kHeaderSize +
      Heap::kArgumentsLengthIndex * kPointerSize;
  __ str(r2, FieldMemOperand(r0, kLengthOffset));

  // Set up the elements pointer in the allocated arguments object.
  // If we allocated a parameter map, r4 will point there, otherwise
  // it will point to the backing store.
  __ add(r4, r0, Operand(Heap::kSloppyArgumentsObjectSize));
  __ str(r4, FieldMemOperand(r0, JSObject::kElementsOffset));

  // r0 = address of new object (tagged)
  // r1 = mapped parameter count (tagged)
  // r2 = argument count (tagged)
  // r4 = address of parameter map or backing store (tagged)
  // Initialize parameter map. If there are no mapped arguments, we're done.
  Label skip_parameter_map;
  __ cmp(r1, Operand(Smi::FromInt(0)));
  // Move backing store address to r3, because it is
  // expected there when filling in the unmapped arguments.
  __ mov(r3, r4, LeaveCC, eq);
  __ b(eq, &skip_parameter_map);

  __ LoadRoot(r6, Heap::kSloppyArgumentsElementsMapRootIndex);
  __ str(r6, FieldMemOperand(r4, FixedArray::kMapOffset));
  __ add(r6, r1, Operand(Smi::FromInt(2)));
  __ str(r6, FieldMemOperand(r4, FixedArray::kLengthOffset));
  __ str(cp, FieldMemOperand(r4, FixedArray::kHeaderSize + 0 * kPointerSize));
  __ add(r6, r4, Operand(r1, LSL, 1));
  __ add(r6, r6, Operand(kParameterMapHeaderSize));
  __ str(r6, FieldMemOperand(r4, FixedArray::kHeaderSize + 1 * kPointerSize));

  // Copy the parameter slots and the holes in the arguments.
  // We need to fill in mapped_parameter_count slots. They index the context,
  // where parameters are stored in reverse order, at
  //   MIN_CONTEXT_SLOTS .. MIN_CONTEXT_SLOTS+parameter_count-1
  // The mapped parameter thus need to get indices
  //   MIN_CONTEXT_SLOTS+parameter_count-1 ..
  //       MIN_CONTEXT_SLOTS+parameter_count-mapped_parameter_count
  // We loop from right to left.
  Label parameters_loop, parameters_test;
  __ mov(r6, r1);
  __ ldr(r9, MemOperand(sp, 0 * kPointerSize));
  __ add(r9, r9, Operand(Smi::FromInt(Context::MIN_CONTEXT_SLOTS)));
  __ sub(r9, r9, Operand(r1));
  __ LoadRoot(r5, Heap::kTheHoleValueRootIndex);
  __ add(r3, r4, Operand(r6, LSL, 1));
  __ add(r3, r3, Operand(kParameterMapHeaderSize));

  // r6 = loop variable (tagged)
  // r1 = mapping index (tagged)
  // r3 = address of backing store (tagged)
  // r4 = address of parameter map (tagged), which is also the address of new
  //      object + Heap::kSloppyArgumentsObjectSize (tagged)
  // r0 = temporary scratch (a.o., for address calculation)
  // r5 = the hole value
  __ jmp(&parameters_test);

  __ bind(&parameters_loop);
  __ sub(r6, r6, Operand(Smi::FromInt(1)));
  __ mov(r0, Operand(r6, LSL, 1));
  __ add(r0, r0, Operand(kParameterMapHeaderSize - kHeapObjectTag));
  __ str(r9, MemOperand(r4, r0));
  __ sub(r0, r0, Operand(kParameterMapHeaderSize - FixedArray::kHeaderSize));
  __ str(r5, MemOperand(r3, r0));
  __ add(r9, r9, Operand(Smi::FromInt(1)));
  __ bind(&parameters_test);
  __ cmp(r6, Operand(Smi::FromInt(0)));
  __ b(ne, &parameters_loop);

  // Restore r0 = new object (tagged)
  __ sub(r0, r4, Operand(Heap::kSloppyArgumentsObjectSize));

  __ bind(&skip_parameter_map);
  // r0 = address of new object (tagged)
  // r2 = argument count (tagged)
  // r3 = address of backing store (tagged)
  // r5 = scratch
  // Copy arguments header and remaining slots (if there are any).
  __ LoadRoot(r5, Heap::kFixedArrayMapRootIndex);
  __ str(r5, FieldMemOperand(r3, FixedArray::kMapOffset));
  __ str(r2, FieldMemOperand(r3, FixedArray::kLengthOffset));

  Label arguments_loop, arguments_test;
  __ mov(r9, r1);
  __ ldr(r4, MemOperand(sp, 1 * kPointerSize));
  __ sub(r4, r4, Operand(r9, LSL, 1));
  __ jmp(&arguments_test);

  __ bind(&arguments_loop);
  __ sub(r4, r4, Operand(kPointerSize));
  __ ldr(r6, MemOperand(r4, 0));
  __ add(r5, r3, Operand(r9, LSL, 1));
  __ str(r6, FieldMemOperand(r5, FixedArray::kHeaderSize));
  __ add(r9, r9, Operand(Smi::FromInt(1)));

  __ bind(&arguments_test);
  __ cmp(r9, Operand(r2));
  __ b(lt, &arguments_loop);

  // Return and remove the on-stack parameters.
  __ add(sp, sp, Operand(3 * kPointerSize));
  __ Ret();

  // Do the runtime call to allocate the arguments object.
  // r0 = address of new object (tagged)
  // r2 = argument count (tagged)
  __ bind(&runtime);
  __ str(r2, MemOperand(sp, 0 * kPointerSize));  // Patch argument count.
  __ TailCallRuntime(Runtime::kNewSloppyArguments, 3, 1);
}


void LoadIndexedInterceptorStub::Generate(MacroAssembler* masm) {
  // Return address is in lr.
  Label slow;

  Register receiver = LoadDescriptor::ReceiverRegister();
  Register key = LoadDescriptor::NameRegister();

  // Check that the key is an array index, that is Uint32.
  __ NonNegativeSmiTst(key);
  __ b(ne, &slow);

  // Everything is fine, call runtime.
  __ Push(receiver, key);  // Receiver, key.

  // Perform tail call to the entry.
  __ TailCallExternalReference(
      ExternalReference(IC_Utility(IC::kLoadElementWithInterceptor),
                        masm->isolate()),
      2, 1);

  __ bind(&slow);
  PropertyAccessCompiler::TailCallBuiltin(
      masm, PropertyAccessCompiler::MissBuiltin(Code::KEYED_LOAD_IC));
}


void ArgumentsAccessStub::GenerateNewStrict(MacroAssembler* masm) {
  // sp[0] : number of parameters
  // sp[4] : receiver displacement
  // sp[8] : function
  // Check if the calling frame is an arguments adaptor frame.
  Label adaptor_frame, try_allocate, runtime;
  __ ldr(r2, MemOperand(fp, StandardFrameConstants::kCallerFPOffset));
  __ ldr(r3, MemOperand(r2, StandardFrameConstants::kContextOffset));
  __ cmp(r3, Operand(Smi::FromInt(StackFrame::ARGUMENTS_ADAPTOR)));
  __ b(eq, &adaptor_frame);

  // Get the length from the frame.
  __ ldr(r1, MemOperand(sp, 0));
  __ b(&try_allocate);

  // Patch the arguments.length and the parameters pointer.
  __ bind(&adaptor_frame);
  __ ldr(r1, MemOperand(r2, ArgumentsAdaptorFrameConstants::kLengthOffset));
  __ str(r1, MemOperand(sp, 0));
  __ add(r3, r2, Operand::PointerOffsetFromSmiKey(r1));
  __ add(r3, r3, Operand(StandardFrameConstants::kCallerSPOffset));
  __ str(r3, MemOperand(sp, 1 * kPointerSize));

  // Try the new space allocation. Start out with computing the size
  // of the arguments object and the elements array in words.
  Label add_arguments_object;
  __ bind(&try_allocate);
  __ SmiUntag(r1, SetCC);
  __ b(eq, &add_arguments_object);
  __ add(r1, r1, Operand(FixedArray::kHeaderSize / kPointerSize));
  __ bind(&add_arguments_object);
  __ add(r1, r1, Operand(Heap::kStrictArgumentsObjectSize / kPointerSize));

  // Do the allocation of both objects in one go.
  __ Allocate(r1, r0, r2, r3, &runtime,
              static_cast<AllocationFlags>(TAG_OBJECT | SIZE_IN_WORDS));

  // Get the arguments boilerplate from the current native context.
  __ ldr(r4, MemOperand(cp, Context::SlotOffset(Context::GLOBAL_OBJECT_INDEX)));
  __ ldr(r4, FieldMemOperand(r4, GlobalObject::kNativeContextOffset));
  __ ldr(r4, MemOperand(
                 r4, Context::SlotOffset(Context::STRICT_ARGUMENTS_MAP_INDEX)));

  __ str(r4, FieldMemOperand(r0, JSObject::kMapOffset));
  __ LoadRoot(r3, Heap::kEmptyFixedArrayRootIndex);
  __ str(r3, FieldMemOperand(r0, JSObject::kPropertiesOffset));
  __ str(r3, FieldMemOperand(r0, JSObject::kElementsOffset));

  // Get the length (smi tagged) and set that as an in-object property too.
  STATIC_ASSERT(Heap::kArgumentsLengthIndex == 0);
  __ ldr(r1, MemOperand(sp, 0 * kPointerSize));
  __ AssertSmi(r1);
  __ str(r1, FieldMemOperand(r0, JSObject::kHeaderSize +
      Heap::kArgumentsLengthIndex * kPointerSize));

  // If there are no actual arguments, we're done.
  Label done;
  __ cmp(r1, Operand::Zero());
  __ b(eq, &done);

  // Get the parameters pointer from the stack.
  __ ldr(r2, MemOperand(sp, 1 * kPointerSize));

  // Set up the elements pointer in the allocated arguments object and
  // initialize the header in the elements fixed array.
  __ add(r4, r0, Operand(Heap::kStrictArgumentsObjectSize));
  __ str(r4, FieldMemOperand(r0, JSObject::kElementsOffset));
  __ LoadRoot(r3, Heap::kFixedArrayMapRootIndex);
  __ str(r3, FieldMemOperand(r4, FixedArray::kMapOffset));
  __ str(r1, FieldMemOperand(r4, FixedArray::kLengthOffset));
  __ SmiUntag(r1);

  // Copy the fixed array slots.
  Label loop;
  // Set up r4 to point to the first array slot.
  __ add(r4, r4, Operand(FixedArray::kHeaderSize - kHeapObjectTag));
  __ bind(&loop);
  // Pre-decrement r2 with kPointerSize on each iteration.
  // Pre-decrement in order to skip receiver.
  __ ldr(r3, MemOperand(r2, kPointerSize, NegPreIndex));
  // Post-increment r4 with kPointerSize on each iteration.
  __ str(r3, MemOperand(r4, kPointerSize, PostIndex));
  __ sub(r1, r1, Operand(1));
  __ cmp(r1, Operand::Zero());
  __ b(ne, &loop);

  // Return and remove the on-stack parameters.
  __ bind(&done);
  __ add(sp, sp, Operand(3 * kPointerSize));
  __ Ret();

  // Do the runtime call to allocate the arguments object.
  __ bind(&runtime);
  __ TailCallRuntime(Runtime::kNewStrictArguments, 3, 1);
}


void RegExpExecStub::Generate(MacroAssembler* masm) {
  // Just jump directly to runtime if native RegExp is not selected at compile
  // time or if regexp entry in generated code is turned off runtime switch or
  // at compilation.
#ifdef V8_INTERPRETED_REGEXP
  __ TailCallRuntime(Runtime::kRegExpExecRT, 4, 1);
#else  // V8_INTERPRETED_REGEXP

  // Stack frame on entry.
  //  sp[0]: last_match_info (expected JSArray)
  //  sp[4]: previous index
  //  sp[8]: subject string
  //  sp[12]: JSRegExp object

  const int kLastMatchInfoOffset = 0 * kPointerSize;
  const int kPreviousIndexOffset = 1 * kPointerSize;
  const int kSubjectOffset = 2 * kPointerSize;
  const int kJSRegExpOffset = 3 * kPointerSize;

  Label runtime;
  // Allocation of registers for this function. These are in callee save
  // registers and will be preserved by the call to the native RegExp code, as
  // this code is called using the normal C calling convention. When calling
  // directly from generated code the native RegExp code will not do a GC and
  // therefore the content of these registers are safe to use after the call.
  Register subject = r4;
  Register regexp_data = r5;
  Register last_match_info_elements = no_reg;  // will be r6;

  // Ensure that a RegExp stack is allocated.
  ExternalReference address_of_regexp_stack_memory_address =
      ExternalReference::address_of_regexp_stack_memory_address(isolate());
  ExternalReference address_of_regexp_stack_memory_size =
      ExternalReference::address_of_regexp_stack_memory_size(isolate());
  __ mov(r0, Operand(address_of_regexp_stack_memory_size));
  __ ldr(r0, MemOperand(r0, 0));
  __ cmp(r0, Operand::Zero());
  __ b(eq, &runtime);

  // Check that the first argument is a JSRegExp object.
  __ ldr(r0, MemOperand(sp, kJSRegExpOffset));
  __ JumpIfSmi(r0, &runtime);
  __ CompareObjectType(r0, r1, r1, JS_REGEXP_TYPE);
  __ b(ne, &runtime);

  // Check that the RegExp has been compiled (data contains a fixed array).
  __ ldr(regexp_data, FieldMemOperand(r0, JSRegExp::kDataOffset));
  if (FLAG_debug_code) {
    __ SmiTst(regexp_data);
    __ Check(ne, kUnexpectedTypeForRegExpDataFixedArrayExpected);
    __ CompareObjectType(regexp_data, r0, r0, FIXED_ARRAY_TYPE);
    __ Check(eq, kUnexpectedTypeForRegExpDataFixedArrayExpected);
  }

  // regexp_data: RegExp data (FixedArray)
  // Check the type of the RegExp. Only continue if type is JSRegExp::IRREGEXP.
  __ ldr(r0, FieldMemOperand(regexp_data, JSRegExp::kDataTagOffset));
  __ cmp(r0, Operand(Smi::FromInt(JSRegExp::IRREGEXP)));
  __ b(ne, &runtime);

  // regexp_data: RegExp data (FixedArray)
  // Check that the number of captures fit in the static offsets vector buffer.
  __ ldr(r2,
         FieldMemOperand(regexp_data, JSRegExp::kIrregexpCaptureCountOffset));
  // Check (number_of_captures + 1) * 2 <= offsets vector size
  // Or          number_of_captures * 2 <= offsets vector size - 2
  // Multiplying by 2 comes for free since r2 is smi-tagged.
  STATIC_ASSERT(kSmiTag == 0);
  STATIC_ASSERT(kSmiTagSize + kSmiShiftSize == 1);
  STATIC_ASSERT(Isolate::kJSRegexpStaticOffsetsVectorSize >= 2);
  __ cmp(r2, Operand(Isolate::kJSRegexpStaticOffsetsVectorSize - 2));
  __ b(hi, &runtime);

  // Reset offset for possibly sliced string.
  __ mov(r9, Operand::Zero());
  __ ldr(subject, MemOperand(sp, kSubjectOffset));
  __ JumpIfSmi(subject, &runtime);
  __ mov(r3, subject);  // Make a copy of the original subject string.
  __ ldr(r0, FieldMemOperand(subject, HeapObject::kMapOffset));
  __ ldrb(r0, FieldMemOperand(r0, Map::kInstanceTypeOffset));
  // subject: subject string
  // r3: subject string
  // r0: subject string instance type
  // regexp_data: RegExp data (FixedArray)
  // Handle subject string according to its encoding and representation:
  // (1) Sequential string?  If yes, go to (5).
  // (2) Anything but sequential or cons?  If yes, go to (6).
  // (3) Cons string.  If the string is flat, replace subject with first string.
  //     Otherwise bailout.
  // (4) Is subject external?  If yes, go to (7).
  // (5) Sequential string.  Load regexp code according to encoding.
  // (E) Carry on.
  /// [...]

  // Deferred code at the end of the stub:
  // (6) Not a long external string?  If yes, go to (8).
  // (7) External string.  Make it, offset-wise, look like a sequential string.
  //     Go to (5).
  // (8) Short external string or not a string?  If yes, bail out to runtime.
  // (9) Sliced string.  Replace subject with parent.  Go to (4).

  Label seq_string /* 5 */, external_string /* 7 */,
        check_underlying /* 4 */, not_seq_nor_cons /* 6 */,
        not_long_external /* 8 */;

  // (1) Sequential string?  If yes, go to (5).
  __ and_(r1,
          r0,
          Operand(kIsNotStringMask |
                  kStringRepresentationMask |
                  kShortExternalStringMask),
          SetCC);
  STATIC_ASSERT((kStringTag | kSeqStringTag) == 0);
  __ b(eq, &seq_string);  // Go to (5).

  // (2) Anything but sequential or cons?  If yes, go to (6).
  STATIC_ASSERT(kConsStringTag < kExternalStringTag);
  STATIC_ASSERT(kSlicedStringTag > kExternalStringTag);
  STATIC_ASSERT(kIsNotStringMask > kExternalStringTag);
  STATIC_ASSERT(kShortExternalStringTag > kExternalStringTag);
  __ cmp(r1, Operand(kExternalStringTag));
  __ b(ge, &not_seq_nor_cons);  // Go to (6).

  // (3) Cons string.  Check that it's flat.
  // Replace subject with first string and reload instance type.
  __ ldr(r0, FieldMemOperand(subject, ConsString::kSecondOffset));
  __ CompareRoot(r0, Heap::kempty_stringRootIndex);
  __ b(ne, &runtime);
  __ ldr(subject, FieldMemOperand(subject, ConsString::kFirstOffset));

  // (4) Is subject external?  If yes, go to (7).
  __ bind(&check_underlying);
  __ ldr(r0, FieldMemOperand(subject, HeapObject::kMapOffset));
  __ ldrb(r0, FieldMemOperand(r0, Map::kInstanceTypeOffset));
  STATIC_ASSERT(kSeqStringTag == 0);
  __ tst(r0, Operand(kStringRepresentationMask));
  // The underlying external string is never a short external string.
  STATIC_ASSERT(ExternalString::kMaxShortLength < ConsString::kMinLength);
  STATIC_ASSERT(ExternalString::kMaxShortLength < SlicedString::kMinLength);
  __ b(ne, &external_string);  // Go to (7).

  // (5) Sequential string.  Load regexp code according to encoding.
  __ bind(&seq_string);
  // subject: sequential subject string (or look-alike, external string)
  // r3: original subject string
  // Load previous index and check range before r3 is overwritten.  We have to
  // use r3 instead of subject here because subject might have been only made
  // to look like a sequential string when it actually is an external string.
  __ ldr(r1, MemOperand(sp, kPreviousIndexOffset));
  __ JumpIfNotSmi(r1, &runtime);
  __ ldr(r3, FieldMemOperand(r3, String::kLengthOffset));
  __ cmp(r3, Operand(r1));
  __ b(ls, &runtime);
  __ SmiUntag(r1);

  STATIC_ASSERT(4 == kOneByteStringTag);
  STATIC_ASSERT(kTwoByteStringTag == 0);
  __ and_(r0, r0, Operand(kStringEncodingMask));
  __ mov(r3, Operand(r0, ASR, 2), SetCC);
  __ ldr(r6, FieldMemOperand(regexp_data, JSRegExp::kDataOneByteCodeOffset),
         ne);
  __ ldr(r6, FieldMemOperand(regexp_data, JSRegExp::kDataUC16CodeOffset), eq);

  // (E) Carry on.  String handling is done.
  // r6: irregexp code
  // Check that the irregexp code has been generated for the actual string
  // encoding. If it has, the field contains a code object otherwise it contains
  // a smi (code flushing support).
  __ JumpIfSmi(r6, &runtime);

  // r1: previous index
  // r3: encoding of subject string (1 if one_byte, 0 if two_byte);
  // r6: code
  // subject: Subject string
  // regexp_data: RegExp data (FixedArray)
  // All checks done. Now push arguments for native regexp code.
  __ IncrementCounter(isolate()->counters()->regexp_entry_native(), 1, r0, r2);

  // Isolates: note we add an additional parameter here (isolate pointer).
  const int kRegExpExecuteArguments = 9;
  const int kParameterRegisters = 4;
  __ EnterExitFrame(false, kRegExpExecuteArguments - kParameterRegisters);

  // Stack pointer now points to cell where return address is to be written.
  // Arguments are before that on the stack or in registers.

  // Argument 9 (sp[20]): Pass current isolate address.
  __ mov(r0, Operand(ExternalReference::isolate_address(isolate())));
  __ str(r0, MemOperand(sp, 5 * kPointerSize));

  // Argument 8 (sp[16]): Indicate that this is a direct call from JavaScript.
  __ mov(r0, Operand(1));
  __ str(r0, MemOperand(sp, 4 * kPointerSize));

  // Argument 7 (sp[12]): Start (high end) of backtracking stack memory area.
  __ mov(r0, Operand(address_of_regexp_stack_memory_address));
  __ ldr(r0, MemOperand(r0, 0));
  __ mov(r2, Operand(address_of_regexp_stack_memory_size));
  __ ldr(r2, MemOperand(r2, 0));
  __ add(r0, r0, Operand(r2));
  __ str(r0, MemOperand(sp, 3 * kPointerSize));

  // Argument 6: Set the number of capture registers to zero to force global
  // regexps to behave as non-global.  This does not affect non-global regexps.
  __ mov(r0, Operand::Zero());
  __ str(r0, MemOperand(sp, 2 * kPointerSize));

  // Argument 5 (sp[4]): static offsets vector buffer.
  __ mov(r0,
         Operand(ExternalReference::address_of_static_offsets_vector(
             isolate())));
  __ str(r0, MemOperand(sp, 1 * kPointerSize));

  // For arguments 4 and 3 get string length, calculate start of string data and
  // calculate the shift of the index (0 for one-byte and 1 for two-byte).
  __ add(r7, subject, Operand(SeqString::kHeaderSize - kHeapObjectTag));
  __ eor(r3, r3, Operand(1));
  // Load the length from the original subject string from the previous stack
  // frame. Therefore we have to use fp, which points exactly to two pointer
  // sizes below the previous sp. (Because creating a new stack frame pushes
  // the previous fp onto the stack and moves up sp by 2 * kPointerSize.)
  __ ldr(subject, MemOperand(fp, kSubjectOffset + 2 * kPointerSize));
  // If slice offset is not 0, load the length from the original sliced string.
  // Argument 4, r3: End of string data
  // Argument 3, r2: Start of string data
  // Prepare start and end index of the input.
  __ add(r9, r7, Operand(r9, LSL, r3));
  __ add(r2, r9, Operand(r1, LSL, r3));

  __ ldr(r7, FieldMemOperand(subject, String::kLengthOffset));
  __ SmiUntag(r7);
  __ add(r3, r9, Operand(r7, LSL, r3));

  // Argument 2 (r1): Previous index.
  // Already there

  // Argument 1 (r0): Subject string.
  __ mov(r0, subject);

  // Locate the code entry and call it.
  __ add(r6, r6, Operand(Code::kHeaderSize - kHeapObjectTag));
  DirectCEntryStub stub(isolate());
  stub.GenerateCall(masm, r6);

  __ LeaveExitFrame(false, no_reg, true);

  last_match_info_elements = r6;

  // r0: result
  // subject: subject string (callee saved)
  // regexp_data: RegExp data (callee saved)
  // last_match_info_elements: Last match info elements (callee saved)
  // Check the result.
  Label success;
  __ cmp(r0, Operand(1));
  // We expect exactly one result since we force the called regexp to behave
  // as non-global.
  __ b(eq, &success);
  Label failure;
  __ cmp(r0, Operand(NativeRegExpMacroAssembler::FAILURE));
  __ b(eq, &failure);
  __ cmp(r0, Operand(NativeRegExpMacroAssembler::EXCEPTION));
  // If not exception it can only be retry. Handle that in the runtime system.
  __ b(ne, &runtime);
  // Result must now be exception. If there is no pending exception already a
  // stack overflow (on the backtrack stack) was detected in RegExp code but
  // haven't created the exception yet. Handle that in the runtime system.
  // TODO(592): Rerunning the RegExp to get the stack overflow exception.
  __ mov(r1, Operand(isolate()->factory()->the_hole_value()));
  __ mov(r2, Operand(ExternalReference(Isolate::kPendingExceptionAddress,
                                       isolate())));
  __ ldr(r0, MemOperand(r2, 0));
  __ cmp(r0, r1);
  __ b(eq, &runtime);

  __ str(r1, MemOperand(r2, 0));  // Clear pending exception.

  // Check if the exception is a termination. If so, throw as uncatchable.
  __ CompareRoot(r0, Heap::kTerminationExceptionRootIndex);

  Label termination_exception;
  __ b(eq, &termination_exception);

  __ Throw(r0);

  __ bind(&termination_exception);
  __ ThrowUncatchable(r0);

  __ bind(&failure);
  // For failure and exception return null.
  __ mov(r0, Operand(isolate()->factory()->null_value()));
  __ add(sp, sp, Operand(4 * kPointerSize));
  __ Ret();

  // Process the result from the native regexp code.
  __ bind(&success);
  __ ldr(r1,
         FieldMemOperand(regexp_data, JSRegExp::kIrregexpCaptureCountOffset));
  // Calculate number of capture registers (number_of_captures + 1) * 2.
  // Multiplying by 2 comes for free since r1 is smi-tagged.
  STATIC_ASSERT(kSmiTag == 0);
  STATIC_ASSERT(kSmiTagSize + kSmiShiftSize == 1);
  __ add(r1, r1, Operand(2));  // r1 was a smi.

  __ ldr(r0, MemOperand(sp, kLastMatchInfoOffset));
  __ JumpIfSmi(r0, &runtime);
  __ CompareObjectType(r0, r2, r2, JS_ARRAY_TYPE);
  __ b(ne, &runtime);
  // Check that the JSArray is in fast case.
  __ ldr(last_match_info_elements,
         FieldMemOperand(r0, JSArray::kElementsOffset));
  __ ldr(r0, FieldMemOperand(last_match_info_elements, HeapObject::kMapOffset));
  __ CompareRoot(r0, Heap::kFixedArrayMapRootIndex);
  __ b(ne, &runtime);
  // Check that the last match info has space for the capture registers and the
  // additional information.
  __ ldr(r0,
         FieldMemOperand(last_match_info_elements, FixedArray::kLengthOffset));
  __ add(r2, r1, Operand(RegExpImpl::kLastMatchOverhead));
  __ cmp(r2, Operand::SmiUntag(r0));
  __ b(gt, &runtime);

  // r1: number of capture registers
  // r4: subject string
  // Store the capture count.
  __ SmiTag(r2, r1);
  __ str(r2, FieldMemOperand(last_match_info_elements,
                             RegExpImpl::kLastCaptureCountOffset));
  // Store last subject and last input.
  __ str(subject,
         FieldMemOperand(last_match_info_elements,
                         RegExpImpl::kLastSubjectOffset));
  __ mov(r2, subject);
  __ RecordWriteField(last_match_info_elements,
                      RegExpImpl::kLastSubjectOffset,
                      subject,
                      r3,
                      kLRHasNotBeenSaved,
                      kDontSaveFPRegs);
  __ mov(subject, r2);
  __ str(subject,
         FieldMemOperand(last_match_info_elements,
                         RegExpImpl::kLastInputOffset));
  __ RecordWriteField(last_match_info_elements,
                      RegExpImpl::kLastInputOffset,
                      subject,
                      r3,
                      kLRHasNotBeenSaved,
                      kDontSaveFPRegs);

  // Get the static offsets vector filled by the native regexp code.
  ExternalReference address_of_static_offsets_vector =
      ExternalReference::address_of_static_offsets_vector(isolate());
  __ mov(r2, Operand(address_of_static_offsets_vector));

  // r1: number of capture registers
  // r2: offsets vector
  Label next_capture, done;
  // Capture register counter starts from number of capture registers and
  // counts down until wraping after zero.
  __ add(r0,
         last_match_info_elements,
         Operand(RegExpImpl::kFirstCaptureOffset - kHeapObjectTag));
  __ bind(&next_capture);
  __ sub(r1, r1, Operand(1), SetCC);
  __ b(mi, &done);
  // Read the value from the static offsets vector buffer.
  __ ldr(r3, MemOperand(r2, kPointerSize, PostIndex));
  // Store the smi value in the last match info.
  __ SmiTag(r3);
  __ str(r3, MemOperand(r0, kPointerSize, PostIndex));
  __ jmp(&next_capture);
  __ bind(&done);

  // Return last match info.
  __ ldr(r0, MemOperand(sp, kLastMatchInfoOffset));
  __ add(sp, sp, Operand(4 * kPointerSize));
  __ Ret();

  // Do the runtime call to execute the regexp.
  __ bind(&runtime);
  __ TailCallRuntime(Runtime::kRegExpExecRT, 4, 1);

  // Deferred code for string handling.
  // (6) Not a long external string?  If yes, go to (8).
  __ bind(&not_seq_nor_cons);
  // Compare flags are still set.
  __ b(gt, &not_long_external);  // Go to (8).

  // (7) External string.  Make it, offset-wise, look like a sequential string.
  __ bind(&external_string);
  __ ldr(r0, FieldMemOperand(subject, HeapObject::kMapOffset));
  __ ldrb(r0, FieldMemOperand(r0, Map::kInstanceTypeOffset));
  if (FLAG_debug_code) {
    // Assert that we do not have a cons or slice (indirect strings) here.
    // Sequential strings have already been ruled out.
    __ tst(r0, Operand(kIsIndirectStringMask));
    __ Assert(eq, kExternalStringExpectedButNotFound);
  }
  __ ldr(subject,
         FieldMemOperand(subject, ExternalString::kResourceDataOffset));
  // Move the pointer so that offset-wise, it looks like a sequential string.
  STATIC_ASSERT(SeqTwoByteString::kHeaderSize == SeqOneByteString::kHeaderSize);
  __ sub(subject,
         subject,
         Operand(SeqTwoByteString::kHeaderSize - kHeapObjectTag));
  __ jmp(&seq_string);    // Go to (5).

  // (8) Short external string or not a string?  If yes, bail out to runtime.
  __ bind(&not_long_external);
  STATIC_ASSERT(kNotStringTag != 0 && kShortExternalStringTag !=0);
  __ tst(r1, Operand(kIsNotStringMask | kShortExternalStringMask));
  __ b(ne, &runtime);

  // (9) Sliced string.  Replace subject with parent.  Go to (4).
  // Load offset into r9 and replace subject string with parent.
  __ ldr(r9, FieldMemOperand(subject, SlicedString::kOffsetOffset));
  __ SmiUntag(r9);
  __ ldr(subject, FieldMemOperand(subject, SlicedString::kParentOffset));
  __ jmp(&check_underlying);  // Go to (4).
#endif  // V8_INTERPRETED_REGEXP
}


static void GenerateRecordCallTarget(MacroAssembler* masm) {
  // Cache the called function in a feedback vector slot.  Cache states
  // are uninitialized, monomorphic (indicated by a JSFunction), and
  // megamorphic.
  // r0 : number of arguments to the construct function
  // r1 : the function to call
  // r2 : Feedback vector
  // r3 : slot in feedback vector (Smi)
  Label initialize, done, miss, megamorphic, not_array_function;

  DCHECK_EQ(*TypeFeedbackVector::MegamorphicSentinel(masm->isolate()),
            masm->isolate()->heap()->megamorphic_symbol());
  DCHECK_EQ(*TypeFeedbackVector::UninitializedSentinel(masm->isolate()),
            masm->isolate()->heap()->uninitialized_symbol());

  // Load the cache state into r4.
  __ add(r4, r2, Operand::PointerOffsetFromSmiKey(r3));
  __ ldr(r4, FieldMemOperand(r4, FixedArray::kHeaderSize));

  // A monomorphic cache hit or an already megamorphic state: invoke the
  // function without changing the state.
  __ cmp(r4, r1);
  __ b(eq, &done);

  if (!FLAG_pretenuring_call_new) {
    // If we came here, we need to see if we are the array function.
    // If we didn't have a matching function, and we didn't find the megamorph
    // sentinel, then we have in the slot either some other function or an
    // AllocationSite. Do a map check on the object in ecx.
    __ ldr(r5, FieldMemOperand(r4, 0));
    __ CompareRoot(r5, Heap::kAllocationSiteMapRootIndex);
    __ b(ne, &miss);

    // Make sure the function is the Array() function
    __ LoadGlobalFunction(Context::ARRAY_FUNCTION_INDEX, r4);
    __ cmp(r1, r4);
    __ b(ne, &megamorphic);
    __ jmp(&done);
  }

  __ bind(&miss);

  // A monomorphic miss (i.e, here the cache is not uninitialized) goes
  // megamorphic.
  __ CompareRoot(r4, Heap::kuninitialized_symbolRootIndex);
  __ b(eq, &initialize);
  // MegamorphicSentinel is an immortal immovable object (undefined) so no
  // write-barrier is needed.
  __ bind(&megamorphic);
  __ add(r4, r2, Operand::PointerOffsetFromSmiKey(r3));
  __ LoadRoot(ip, Heap::kmegamorphic_symbolRootIndex);
  __ str(ip, FieldMemOperand(r4, FixedArray::kHeaderSize));
  __ jmp(&done);

  // An uninitialized cache is patched with the function
  __ bind(&initialize);

  if (!FLAG_pretenuring_call_new) {
    // Make sure the function is the Array() function
    __ LoadGlobalFunction(Context::ARRAY_FUNCTION_INDEX, r4);
    __ cmp(r1, r4);
    __ b(ne, &not_array_function);

    // The target function is the Array constructor,
    // Create an AllocationSite if we don't already have it, store it in the
    // slot.
    {
      FrameAndConstantPoolScope scope(masm, StackFrame::INTERNAL);

      // Arguments register must be smi-tagged to call out.
      __ SmiTag(r0);
      __ Push(r3, r2, r1, r0);

      CreateAllocationSiteStub create_stub(masm->isolate());
      __ CallStub(&create_stub);

      __ Pop(r3, r2, r1, r0);
      __ SmiUntag(r0);
    }
    __ b(&done);

    __ bind(&not_array_function);
  }

  __ add(r4, r2, Operand::PointerOffsetFromSmiKey(r3));
  __ add(r4, r4, Operand(FixedArray::kHeaderSize - kHeapObjectTag));
  __ str(r1, MemOperand(r4, 0));

  __ Push(r4, r2, r1);
  __ RecordWrite(r2, r4, r1, kLRHasNotBeenSaved, kDontSaveFPRegs,
                 EMIT_REMEMBERED_SET, OMIT_SMI_CHECK);
  __ Pop(r4, r2, r1);

  __ bind(&done);
}


static void EmitContinueIfStrictOrNative(MacroAssembler* masm, Label* cont) {
  // Do not transform the receiver for strict mode functions.
  __ ldr(r3, FieldMemOperand(r1, JSFunction::kSharedFunctionInfoOffset));
  __ ldr(r4, FieldMemOperand(r3, SharedFunctionInfo::kCompilerHintsOffset));
  __ tst(r4, Operand(1 << (SharedFunctionInfo::kStrictModeFunction +
                           kSmiTagSize)));
  __ b(ne, cont);

  // Do not transform the receiver for native (Compilerhints already in r3).
  __ tst(r4, Operand(1 << (SharedFunctionInfo::kNative + kSmiTagSize)));
  __ b(ne, cont);
}


static void EmitSlowCase(MacroAssembler* masm,
                         int argc,
                         Label* non_function) {
  // Check for function proxy.
  __ cmp(r4, Operand(JS_FUNCTION_PROXY_TYPE));
  __ b(ne, non_function);
  __ push(r1);  // put proxy as additional argument
  __ mov(r0, Operand(argc + 1, RelocInfo::NONE32));
  __ mov(r2, Operand::Zero());
  __ GetBuiltinFunction(r1, Builtins::CALL_FUNCTION_PROXY);
  {
    Handle<Code> adaptor =
        masm->isolate()->builtins()->ArgumentsAdaptorTrampoline();
    __ Jump(adaptor, RelocInfo::CODE_TARGET);
  }

  // CALL_NON_FUNCTION expects the non-function callee as receiver (instead
  // of the original receiver from the call site).
  __ bind(non_function);
  __ str(r1, MemOperand(sp, argc * kPointerSize));
  __ mov(r0, Operand(argc));  // Set up the number of arguments.
  __ mov(r2, Operand::Zero());
  __ GetBuiltinFunction(r1, Builtins::CALL_NON_FUNCTION);
  __ Jump(masm->isolate()->builtins()->ArgumentsAdaptorTrampoline(),
          RelocInfo::CODE_TARGET);
}


static void EmitWrapCase(MacroAssembler* masm, int argc, Label* cont) {
  // Wrap the receiver and patch it back onto the stack.
  { FrameAndConstantPoolScope frame_scope(masm, StackFrame::INTERNAL);
    __ Push(r1, r3);
    __ InvokeBuiltin(Builtins::TO_OBJECT, CALL_FUNCTION);
    __ pop(r1);
  }
  __ str(r0, MemOperand(sp, argc * kPointerSize));
  __ jmp(cont);
}


static void CallFunctionNoFeedback(MacroAssembler* masm,
                                   int argc, bool needs_checks,
                                   bool call_as_method) {
  // r1 : the function to call
  Label slow, non_function, wrap, cont;

  if (needs_checks) {
    // Check that the function is really a JavaScript function.
    // r1: pushed function (to be verified)
    __ JumpIfSmi(r1, &non_function);

    // Goto slow case if we do not have a function.
    __ CompareObjectType(r1, r4, r4, JS_FUNCTION_TYPE);
    __ b(ne, &slow);
  }

  // Fast-case: Invoke the function now.
  // r1: pushed function
  ParameterCount actual(argc);

  if (call_as_method) {
    if (needs_checks) {
      EmitContinueIfStrictOrNative(masm, &cont);
    }

    // Compute the receiver in sloppy mode.
    __ ldr(r3, MemOperand(sp, argc * kPointerSize));

    if (needs_checks) {
      __ JumpIfSmi(r3, &wrap);
      __ CompareObjectType(r3, r4, r4, FIRST_SPEC_OBJECT_TYPE);
      __ b(lt, &wrap);
    } else {
      __ jmp(&wrap);
    }

    __ bind(&cont);
  }

  __ InvokeFunction(r1, actual, JUMP_FUNCTION, NullCallWrapper());

  if (needs_checks) {
    // Slow-case: Non-function called.
    __ bind(&slow);
    EmitSlowCase(masm, argc, &non_function);
  }

  if (call_as_method) {
    __ bind(&wrap);
    EmitWrapCase(masm, argc, &cont);
  }
}


void CallFunctionStub::Generate(MacroAssembler* masm) {
  CallFunctionNoFeedback(masm, argc(), NeedsChecks(), CallAsMethod());
}


void CallConstructStub::Generate(MacroAssembler* masm) {
  // r0 : number of arguments
  // r1 : the function to call
  // r2 : feedback vector
  // r3 : (only if r2 is not the megamorphic symbol) slot in feedback
  //      vector (Smi)
  Label slow, non_function_call;

  // Check that the function is not a smi.
  __ JumpIfSmi(r1, &non_function_call);
  // Check that the function is a JSFunction.
  __ CompareObjectType(r1, r4, r4, JS_FUNCTION_TYPE);
  __ b(ne, &slow);

  if (RecordCallTarget()) {
    GenerateRecordCallTarget(masm);

    __ add(r5, r2, Operand::PointerOffsetFromSmiKey(r3));
    if (FLAG_pretenuring_call_new) {
      // Put the AllocationSite from the feedback vector into r2.
      // By adding kPointerSize we encode that we know the AllocationSite
      // entry is at the feedback vector slot given by r3 + 1.
      __ ldr(r2, FieldMemOperand(r5, FixedArray::kHeaderSize + kPointerSize));
    } else {
      Label feedback_register_initialized;
      // Put the AllocationSite from the feedback vector into r2, or undefined.
      __ ldr(r2, FieldMemOperand(r5, FixedArray::kHeaderSize));
      __ ldr(r5, FieldMemOperand(r2, AllocationSite::kMapOffset));
      __ CompareRoot(r5, Heap::kAllocationSiteMapRootIndex);
      __ b(eq, &feedback_register_initialized);
      __ LoadRoot(r2, Heap::kUndefinedValueRootIndex);
      __ bind(&feedback_register_initialized);
    }

    __ AssertUndefinedOrAllocationSite(r2, r5);
  }

  // Jump to the function-specific construct stub.
  Register jmp_reg = r4;
  __ ldr(jmp_reg, FieldMemOperand(r1, JSFunction::kSharedFunctionInfoOffset));
  __ ldr(jmp_reg, FieldMemOperand(jmp_reg,
                                  SharedFunctionInfo::kConstructStubOffset));
  __ add(pc, jmp_reg, Operand(Code::kHeaderSize - kHeapObjectTag));

  // r0: number of arguments
  // r1: called object
  // r4: object type
  Label do_call;
  __ bind(&slow);
  __ cmp(r4, Operand(JS_FUNCTION_PROXY_TYPE));
  __ b(ne, &non_function_call);
  __ GetBuiltinFunction(r1, Builtins::CALL_FUNCTION_PROXY_AS_CONSTRUCTOR);
  __ jmp(&do_call);

  __ bind(&non_function_call);
  __ GetBuiltinFunction(r1, Builtins::CALL_NON_FUNCTION_AS_CONSTRUCTOR);
  __ bind(&do_call);
  // Set expected number of arguments to zero (not changing r0).
  __ mov(r2, Operand::Zero());
  __ Jump(masm->isolate()->builtins()->ArgumentsAdaptorTrampoline(),
          RelocInfo::CODE_TARGET);
}


static void EmitLoadTypeFeedbackVector(MacroAssembler* masm, Register vector) {
  __ ldr(vector, MemOperand(fp, JavaScriptFrameConstants::kFunctionOffset));
  __ ldr(vector, FieldMemOperand(vector,
                                 JSFunction::kSharedFunctionInfoOffset));
  __ ldr(vector, FieldMemOperand(vector,
                                 SharedFunctionInfo::kFeedbackVectorOffset));
}


void CallIC_ArrayStub::Generate(MacroAssembler* masm) {
  // r1 - function
  // r3 - slot id
  Label miss;
  int argc = arg_count();
  ParameterCount actual(argc);

  EmitLoadTypeFeedbackVector(masm, r2);

  __ LoadGlobalFunction(Context::ARRAY_FUNCTION_INDEX, r4);
  __ cmp(r1, r4);
  __ b(ne, &miss);

  __ mov(r0, Operand(arg_count()));
  __ add(r4, r2, Operand::PointerOffsetFromSmiKey(r3));
  __ ldr(r4, FieldMemOperand(r4, FixedArray::kHeaderSize));

  // Verify that r4 contains an AllocationSite
  __ ldr(r5, FieldMemOperand(r4, HeapObject::kMapOffset));
  __ CompareRoot(r5, Heap::kAllocationSiteMapRootIndex);
  __ b(ne, &miss);

  __ mov(r2, r4);
  ArrayConstructorStub stub(masm->isolate(), arg_count());
  __ TailCallStub(&stub);

  __ bind(&miss);
  GenerateMiss(masm);

  // The slow case, we need this no matter what to complete a call after a miss.
  CallFunctionNoFeedback(masm,
                         arg_count(),
                         true,
                         CallAsMethod());

  // Unreachable.
  __ stop("Unexpected code address");
}


void CallICStub::Generate(MacroAssembler* masm) {
  // r1 - function
  // r3 - slot id (Smi)
  Label extra_checks_or_miss, slow_start;
  Label slow, non_function, wrap, cont;
  Label have_js_function;
  int argc = arg_count();
  ParameterCount actual(argc);

  EmitLoadTypeFeedbackVector(masm, r2);

  // The checks. First, does r1 match the recorded monomorphic target?
  __ add(r4, r2, Operand::PointerOffsetFromSmiKey(r3));
  __ ldr(r4, FieldMemOperand(r4, FixedArray::kHeaderSize));
  __ cmp(r1, r4);
  __ b(ne, &extra_checks_or_miss);

  __ bind(&have_js_function);
  if (CallAsMethod()) {
    EmitContinueIfStrictOrNative(masm, &cont);
    // Compute the receiver in sloppy mode.
    __ ldr(r3, MemOperand(sp, argc * kPointerSize));

    __ JumpIfSmi(r3, &wrap);
    __ CompareObjectType(r3, r4, r4, FIRST_SPEC_OBJECT_TYPE);
    __ b(lt, &wrap);

    __ bind(&cont);
  }

  __ InvokeFunction(r1, actual, JUMP_FUNCTION, NullCallWrapper());

  __ bind(&slow);
  EmitSlowCase(masm, argc, &non_function);

  if (CallAsMethod()) {
    __ bind(&wrap);
    EmitWrapCase(masm, argc, &cont);
  }

  __ bind(&extra_checks_or_miss);
  Label miss;

  __ CompareRoot(r4, Heap::kmegamorphic_symbolRootIndex);
  __ b(eq, &slow_start);
  __ CompareRoot(r4, Heap::kuninitialized_symbolRootIndex);
  __ b(eq, &miss);

  if (!FLAG_trace_ic) {
    // We are going megamorphic. If the feedback is a JSFunction, it is fine
    // to handle it here. More complex cases are dealt with in the runtime.
    __ AssertNotSmi(r4);
    __ CompareObjectType(r4, r5, r5, JS_FUNCTION_TYPE);
    __ b(ne, &miss);
    __ add(r4, r2, Operand::PointerOffsetFromSmiKey(r3));
    __ LoadRoot(ip, Heap::kmegamorphic_symbolRootIndex);
    __ str(ip, FieldMemOperand(r4, FixedArray::kHeaderSize));
    // We have to update statistics for runtime profiling.
    const int with_types_offset =
        FixedArray::OffsetOfElementAt(TypeFeedbackVector::kWithTypesIndex);
    __ ldr(r4, FieldMemOperand(r2, with_types_offset));
    __ sub(r4, r4, Operand(Smi::FromInt(1)));
    __ str(r4, FieldMemOperand(r2, with_types_offset));
    const int generic_offset =
        FixedArray::OffsetOfElementAt(TypeFeedbackVector::kGenericCountIndex);
    __ ldr(r4, FieldMemOperand(r2, generic_offset));
    __ add(r4, r4, Operand(Smi::FromInt(1)));
    __ str(r4, FieldMemOperand(r2, generic_offset));
    __ jmp(&slow_start);
  }

  // We are here because tracing is on or we are going monomorphic.
  __ bind(&miss);
  GenerateMiss(masm);

  // the slow case
  __ bind(&slow_start);
  // Check that the function is really a JavaScript function.
  // r1: pushed function (to be verified)
  __ JumpIfSmi(r1, &non_function);

  // Goto slow case if we do not have a function.
  __ CompareObjectType(r1, r4, r4, JS_FUNCTION_TYPE);
  __ b(ne, &slow);
  __ jmp(&have_js_function);
}


void CallICStub::GenerateMiss(MacroAssembler* masm) {
  // Get the receiver of the function from the stack; 1 ~ return address.
  __ ldr(r4, MemOperand(sp, (arg_count() + 1) * kPointerSize));

  {
    FrameAndConstantPoolScope scope(masm, StackFrame::INTERNAL);

    // Push the receiver and the function and feedback info.
    __ Push(r4, r1, r2, r3);

    // Call the entry.
    IC::UtilityId id = GetICState() == DEFAULT ? IC::kCallIC_Miss
                                               : IC::kCallIC_Customization_Miss;

    ExternalReference miss = ExternalReference(IC_Utility(id),
                                               masm->isolate());
    __ CallExternalReference(miss, 4);

    // Move result to edi and exit the internal frame.
    __ mov(r1, r0);
  }
}


// StringCharCodeAtGenerator
void StringCharCodeAtGenerator::GenerateFast(MacroAssembler* masm) {
  // If the receiver is a smi trigger the non-string case.
  if (check_mode_ == RECEIVER_IS_UNKNOWN) {
    __ JumpIfSmi(object_, receiver_not_string_);

    // Fetch the instance type of the receiver into result register.
    __ ldr(result_, FieldMemOperand(object_, HeapObject::kMapOffset));
    __ ldrb(result_, FieldMemOperand(result_, Map::kInstanceTypeOffset));
    // If the receiver is not a string trigger the non-string case.
    __ tst(result_, Operand(kIsNotStringMask));
    __ b(ne, receiver_not_string_);
  }

  // If the index is non-smi trigger the non-smi case.
  __ JumpIfNotSmi(index_, &index_not_smi_);
  __ bind(&got_smi_index_);

  // Check for index out of range.
  __ ldr(ip, FieldMemOperand(object_, String::kLengthOffset));
  __ cmp(ip, Operand(index_));
  __ b(ls, index_out_of_range_);

  __ SmiUntag(index_);

  StringCharLoadGenerator::Generate(masm,
                                    object_,
                                    index_,
                                    result_,
                                    &call_runtime_);

  __ SmiTag(result_);
  __ bind(&exit_);
}


void StringCharCodeAtGenerator::GenerateSlow(
    MacroAssembler* masm,
    const RuntimeCallHelper& call_helper) {
  __ Abort(kUnexpectedFallthroughToCharCodeAtSlowCase);

  // Index is not a smi.
  __ bind(&index_not_smi_);
  // If index is a heap number, try converting it to an integer.
  __ CheckMap(index_,
              result_,
              Heap::kHeapNumberMapRootIndex,
              index_not_number_,
              DONT_DO_SMI_CHECK);
  call_helper.BeforeCall(masm);
  __ push(object_);
  __ push(index_);  // Consumed by runtime conversion function.
  if (index_flags_ == STRING_INDEX_IS_NUMBER) {
    __ CallRuntime(Runtime::kNumberToIntegerMapMinusZero, 1);
  } else {
    DCHECK(index_flags_ == STRING_INDEX_IS_ARRAY_INDEX);
    // NumberToSmi discards numbers that are not exact integers.
    __ CallRuntime(Runtime::kNumberToSmi, 1);
  }
  // Save the conversion result before the pop instructions below
  // have a chance to overwrite it.
  __ Move(index_, r0);
  __ pop(object_);
  // Reload the instance type.
  __ ldr(result_, FieldMemOperand(object_, HeapObject::kMapOffset));
  __ ldrb(result_, FieldMemOperand(result_, Map::kInstanceTypeOffset));
  call_helper.AfterCall(masm);
  // If index is still not a smi, it must be out of range.
  __ JumpIfNotSmi(index_, index_out_of_range_);
  // Otherwise, return to the fast path.
  __ jmp(&got_smi_index_);

  // Call runtime. We get here when the receiver is a string and the
  // index is a number, but the code of getting the actual character
  // is too complex (e.g., when the string needs to be flattened).
  __ bind(&call_runtime_);
  call_helper.BeforeCall(masm);
  __ SmiTag(index_);
  __ Push(object_, index_);
  __ CallRuntime(Runtime::kStringCharCodeAtRT, 2);
  __ Move(result_, r0);
  call_helper.AfterCall(masm);
  __ jmp(&exit_);

  __ Abort(kUnexpectedFallthroughFromCharCodeAtSlowCase);
}


// -------------------------------------------------------------------------
// StringCharFromCodeGenerator

void StringCharFromCodeGenerator::GenerateFast(MacroAssembler* masm) {
  // Fast case of Heap::LookupSingleCharacterStringFromCode.
  STATIC_ASSERT(kSmiTag == 0);
  STATIC_ASSERT(kSmiShiftSize == 0);
  DCHECK(base::bits::IsPowerOfTwo32(String::kMaxOneByteCharCode + 1));
  __ tst(code_,
         Operand(kSmiTagMask |
                 ((~String::kMaxOneByteCharCode) << kSmiTagSize)));
  __ b(ne, &slow_case_);

  __ LoadRoot(result_, Heap::kSingleCharacterStringCacheRootIndex);
  // At this point code register contains smi tagged one-byte char code.
  __ add(result_, result_, Operand::PointerOffsetFromSmiKey(code_));
  __ ldr(result_, FieldMemOperand(result_, FixedArray::kHeaderSize));
  __ CompareRoot(result_, Heap::kUndefinedValueRootIndex);
  __ b(eq, &slow_case_);
  __ bind(&exit_);
}


void StringCharFromCodeGenerator::GenerateSlow(
    MacroAssembler* masm,
    const RuntimeCallHelper& call_helper) {
  __ Abort(kUnexpectedFallthroughToCharFromCodeSlowCase);

  __ bind(&slow_case_);
  call_helper.BeforeCall(masm);
  __ push(code_);
  __ CallRuntime(Runtime::kCharFromCode, 1);
  __ Move(result_, r0);
  call_helper.AfterCall(masm);
  __ jmp(&exit_);

  __ Abort(kUnexpectedFallthroughFromCharFromCodeSlowCase);
}


enum CopyCharactersFlags { COPY_ONE_BYTE = 1, DEST_ALWAYS_ALIGNED = 2 };


void StringHelper::GenerateCopyCharacters(MacroAssembler* masm,
                                          Register dest,
                                          Register src,
                                          Register count,
                                          Register scratch,
                                          String::Encoding encoding) {
  if (FLAG_debug_code) {
    // Check that destination is word aligned.
    __ tst(dest, Operand(kPointerAlignmentMask));
    __ Check(eq, kDestinationOfCopyNotAligned);
  }

  // Assumes word reads and writes are little endian.
  // Nothing to do for zero characters.
  Label done;
  if (encoding == String::TWO_BYTE_ENCODING) {
    __ add(count, count, Operand(count), SetCC);
  }

  Register limit = count;  // Read until dest equals this.
  __ add(limit, dest, Operand(count));

  Label loop_entry, loop;
  // Copy bytes from src to dest until dest hits limit.
  __ b(&loop_entry);
  __ bind(&loop);
  __ ldrb(scratch, MemOperand(src, 1, PostIndex), lt);
  __ strb(scratch, MemOperand(dest, 1, PostIndex));
  __ bind(&loop_entry);
  __ cmp(dest, Operand(limit));
  __ b(lt, &loop);

  __ bind(&done);
}


void SubStringStub::Generate(MacroAssembler* masm) {
  Label runtime;

  // Stack frame on entry.
  //  lr: return address
  //  sp[0]: to
  //  sp[4]: from
  //  sp[8]: string

  // This stub is called from the native-call %_SubString(...), so
  // nothing can be assumed about the arguments. It is tested that:
  //  "string" is a sequential string,
  //  both "from" and "to" are smis, and
  //  0 <= from <= to <= string.length.
  // If any of these assumptions fail, we call the runtime system.

  const int kToOffset = 0 * kPointerSize;
  const int kFromOffset = 1 * kPointerSize;
  const int kStringOffset = 2 * kPointerSize;

  __ Ldrd(r2, r3, MemOperand(sp, kToOffset));
  STATIC_ASSERT(kFromOffset == kToOffset + 4);
  STATIC_ASSERT(kSmiTag == 0);
  STATIC_ASSERT(kSmiTagSize + kSmiShiftSize == 1);

  // Arithmetic shift right by one un-smi-tags. In this case we rotate right
  // instead because we bail out on non-smi values: ROR and ASR are equivalent
  // for smis but they set the flags in a way that's easier to optimize.
  __ mov(r2, Operand(r2, ROR, 1), SetCC);
  __ mov(r3, Operand(r3, ROR, 1), SetCC, cc);
  // If either to or from had the smi tag bit set, then C is set now, and N
  // has the same value: we rotated by 1, so the bottom bit is now the top bit.
  // We want to bailout to runtime here if From is negative.  In that case, the
  // next instruction is not executed and we fall through to bailing out to
  // runtime.
  // Executed if both r2 and r3 are untagged integers.
  __ sub(r2, r2, Operand(r3), SetCC, cc);
  // One of the above un-smis or the above SUB could have set N==1.
  __ b(mi, &runtime);  // Either "from" or "to" is not an smi, or from > to.

  // Make sure first argument is a string.
  __ ldr(r0, MemOperand(sp, kStringOffset));
  __ JumpIfSmi(r0, &runtime);
  Condition is_string = masm->IsObjectStringType(r0, r1);
  __ b(NegateCondition(is_string), &runtime);

  Label single_char;
  __ cmp(r2, Operand(1));
  __ b(eq, &single_char);

  // Short-cut for the case of trivial substring.
  Label return_r0;
  // r0: original string
  // r2: result string length
  __ ldr(r4, FieldMemOperand(r0, String::kLengthOffset));
  __ cmp(r2, Operand(r4, ASR, 1));
  // Return original string.
  __ b(eq, &return_r0);
  // Longer than original string's length or negative: unsafe arguments.
  __ b(hi, &runtime);
  // Shorter than original string's length: an actual substring.

  // Deal with different string types: update the index if necessary
  // and put the underlying string into r5.
  // r0: original string
  // r1: instance type
  // r2: length
  // r3: from index (untagged)
  Label underlying_unpacked, sliced_string, seq_or_external_string;
  // If the string is not indirect, it can only be sequential or external.
  STATIC_ASSERT(kIsIndirectStringMask == (kSlicedStringTag & kConsStringTag));
  STATIC_ASSERT(kIsIndirectStringMask != 0);
  __ tst(r1, Operand(kIsIndirectStringMask));
  __ b(eq, &seq_or_external_string);

  __ tst(r1, Operand(kSlicedNotConsMask));
  __ b(ne, &sliced_string);
  // Cons string.  Check whether it is flat, then fetch first part.
  __ ldr(r5, FieldMemOperand(r0, ConsString::kSecondOffset));
  __ CompareRoot(r5, Heap::kempty_stringRootIndex);
  __ b(ne, &runtime);
  __ ldr(r5, FieldMemOperand(r0, ConsString::kFirstOffset));
  // Update instance type.
  __ ldr(r1, FieldMemOperand(r5, HeapObject::kMapOffset));
  __ ldrb(r1, FieldMemOperand(r1, Map::kInstanceTypeOffset));
  __ jmp(&underlying_unpacked);

  __ bind(&sliced_string);
  // Sliced string.  Fetch parent and correct start index by offset.
  __ ldr(r5, FieldMemOperand(r0, SlicedString::kParentOffset));
  __ ldr(r4, FieldMemOperand(r0, SlicedString::kOffsetOffset));
  __ add(r3, r3, Operand(r4, ASR, 1));  // Add offset to index.
  // Update instance type.
  __ ldr(r1, FieldMemOperand(r5, HeapObject::kMapOffset));
  __ ldrb(r1, FieldMemOperand(r1, Map::kInstanceTypeOffset));
  __ jmp(&underlying_unpacked);

  __ bind(&seq_or_external_string);
  // Sequential or external string.  Just move string to the expected register.
  __ mov(r5, r0);

  __ bind(&underlying_unpacked);

  if (FLAG_string_slices) {
    Label copy_routine;
    // r5: underlying subject string
    // r1: instance type of underlying subject string
    // r2: length
    // r3: adjusted start index (untagged)
    __ cmp(r2, Operand(SlicedString::kMinLength));
    // Short slice.  Copy instead of slicing.
    __ b(lt, &copy_routine);
    // Allocate new sliced string.  At this point we do not reload the instance
    // type including the string encoding because we simply rely on the info
    // provided by the original string.  It does not matter if the original
    // string's encoding is wrong because we always have to recheck encoding of
    // the newly created string's parent anyways due to externalized strings.
    Label two_byte_slice, set_slice_header;
    STATIC_ASSERT((kStringEncodingMask & kOneByteStringTag) != 0);
    STATIC_ASSERT((kStringEncodingMask & kTwoByteStringTag) == 0);
    __ tst(r1, Operand(kStringEncodingMask));
    __ b(eq, &two_byte_slice);
    __ AllocateOneByteSlicedString(r0, r2, r6, r4, &runtime);
    __ jmp(&set_slice_header);
    __ bind(&two_byte_slice);
    __ AllocateTwoByteSlicedString(r0, r2, r6, r4, &runtime);
    __ bind(&set_slice_header);
    __ mov(r3, Operand(r3, LSL, 1));
    __ str(r5, FieldMemOperand(r0, SlicedString::kParentOffset));
    __ str(r3, FieldMemOperand(r0, SlicedString::kOffsetOffset));
    __ jmp(&return_r0);

    __ bind(&copy_routine);
  }

  // r5: underlying subject string
  // r1: instance type of underlying subject string
  // r2: length
  // r3: adjusted start index (untagged)
  Label two_byte_sequential, sequential_string, allocate_result;
  STATIC_ASSERT(kExternalStringTag != 0);
  STATIC_ASSERT(kSeqStringTag == 0);
  __ tst(r1, Operand(kExternalStringTag));
  __ b(eq, &sequential_string);

  // Handle external string.
  // Rule out short external strings.
  STATIC_ASSERT(kShortExternalStringTag != 0);
  __ tst(r1, Operand(kShortExternalStringTag));
  __ b(ne, &runtime);
  __ ldr(r5, FieldMemOperand(r5, ExternalString::kResourceDataOffset));
  // r5 already points to the first character of underlying string.
  __ jmp(&allocate_result);

  __ bind(&sequential_string);
  // Locate first character of underlying subject string.
  STATIC_ASSERT(SeqTwoByteString::kHeaderSize == SeqOneByteString::kHeaderSize);
  __ add(r5, r5, Operand(SeqOneByteString::kHeaderSize - kHeapObjectTag));

  __ bind(&allocate_result);
  // Sequential acii string.  Allocate the result.
  STATIC_ASSERT((kOneByteStringTag & kStringEncodingMask) != 0);
  __ tst(r1, Operand(kStringEncodingMask));
  __ b(eq, &two_byte_sequential);

  // Allocate and copy the resulting one-byte string.
  __ AllocateOneByteString(r0, r2, r4, r6, r1, &runtime);

  // Locate first character of substring to copy.
  __ add(r5, r5, r3);
  // Locate first character of result.
  __ add(r1, r0, Operand(SeqOneByteString::kHeaderSize - kHeapObjectTag));

  // r0: result string
  // r1: first character of result string
  // r2: result string length
  // r5: first character of substring to copy
  STATIC_ASSERT((SeqOneByteString::kHeaderSize & kObjectAlignmentMask) == 0);
  StringHelper::GenerateCopyCharacters(
      masm, r1, r5, r2, r3, String::ONE_BYTE_ENCODING);
  __ jmp(&return_r0);

  // Allocate and copy the resulting two-byte string.
  __ bind(&two_byte_sequential);
  __ AllocateTwoByteString(r0, r2, r4, r6, r1, &runtime);

  // Locate first character of substring to copy.
  STATIC_ASSERT(kSmiTagSize == 1 && kSmiTag == 0);
  __ add(r5, r5, Operand(r3, LSL, 1));
  // Locate first character of result.
  __ add(r1, r0, Operand(SeqTwoByteString::kHeaderSize - kHeapObjectTag));

  // r0: result string.
  // r1: first character of result.
  // r2: result length.
  // r5: first character of substring to copy.
  STATIC_ASSERT((SeqTwoByteString::kHeaderSize & kObjectAlignmentMask) == 0);
  StringHelper::GenerateCopyCharacters(
      masm, r1, r5, r2, r3, String::TWO_BYTE_ENCODING);

  __ bind(&return_r0);
  Counters* counters = isolate()->counters();
  __ IncrementCounter(counters->sub_string_native(), 1, r3, r4);
  __ Drop(3);
  __ Ret();

  // Just jump to runtime to create the sub string.
  __ bind(&runtime);
  __ TailCallRuntime(Runtime::kSubString, 3, 1);

  __ bind(&single_char);
  // r0: original string
  // r1: instance type
  // r2: length
  // r3: from index (untagged)
  __ SmiTag(r3, r3);
  StringCharAtGenerator generator(r0, r3, r2, r0, &runtime, &runtime, &runtime,
                                  STRING_INDEX_IS_NUMBER, RECEIVER_IS_STRING);
  generator.GenerateFast(masm);
  __ Drop(3);
  __ Ret();
  generator.SkipSlow(masm, &runtime);
}


void StringHelper::GenerateFlatOneByteStringEquals(
    MacroAssembler* masm, Register left, Register right, Register scratch1,
    Register scratch2, Register scratch3) {
  Register length = scratch1;

  // Compare lengths.
  Label strings_not_equal, check_zero_length;
  __ ldr(length, FieldMemOperand(left, String::kLengthOffset));
  __ ldr(scratch2, FieldMemOperand(right, String::kLengthOffset));
  __ cmp(length, scratch2);
  __ b(eq, &check_zero_length);
  __ bind(&strings_not_equal);
  __ mov(r0, Operand(Smi::FromInt(NOT_EQUAL)));
  __ Ret();

  // Check if the length is zero.
  Label compare_chars;
  __ bind(&check_zero_length);
  STATIC_ASSERT(kSmiTag == 0);
  __ cmp(length, Operand::Zero());
  __ b(ne, &compare_chars);
  __ mov(r0, Operand(Smi::FromInt(EQUAL)));
  __ Ret();

  // Compare characters.
  __ bind(&compare_chars);
  GenerateOneByteCharsCompareLoop(masm, left, right, length, scratch2, scratch3,
                                  &strings_not_equal);

  // Characters are equal.
  __ mov(r0, Operand(Smi::FromInt(EQUAL)));
  __ Ret();
}


void StringHelper::GenerateCompareFlatOneByteStrings(
    MacroAssembler* masm, Register left, Register right, Register scratch1,
    Register scratch2, Register scratch3, Register scratch4) {
  Label result_not_equal, compare_lengths;
  // Find minimum length and length difference.
  __ ldr(scratch1, FieldMemOperand(left, String::kLengthOffset));
  __ ldr(scratch2, FieldMemOperand(right, String::kLengthOffset));
  __ sub(scratch3, scratch1, Operand(scratch2), SetCC);
  Register length_delta = scratch3;
  __ mov(scratch1, scratch2, LeaveCC, gt);
  Register min_length = scratch1;
  STATIC_ASSERT(kSmiTag == 0);
  __ cmp(min_length, Operand::Zero());
  __ b(eq, &compare_lengths);

  // Compare loop.
  GenerateOneByteCharsCompareLoop(masm, left, right, min_length, scratch2,
                                  scratch4, &result_not_equal);

  // Compare lengths - strings up to min-length are equal.
  __ bind(&compare_lengths);
  DCHECK(Smi::FromInt(EQUAL) == static_cast<Smi*>(0));
  // Use length_delta as result if it's zero.
  __ mov(r0, Operand(length_delta), SetCC);
  __ bind(&result_not_equal);
  // Conditionally update the result based either on length_delta or
  // the last comparion performed in the loop above.
  __ mov(r0, Operand(Smi::FromInt(GREATER)), LeaveCC, gt);
  __ mov(r0, Operand(Smi::FromInt(LESS)), LeaveCC, lt);
  __ Ret();
}


void StringHelper::GenerateOneByteCharsCompareLoop(
    MacroAssembler* masm, Register left, Register right, Register length,
    Register scratch1, Register scratch2, Label* chars_not_equal) {
  // Change index to run from -length to -1 by adding length to string
  // start. This means that loop ends when index reaches zero, which
  // doesn't need an additional compare.
  __ SmiUntag(length);
  __ add(scratch1, length,
         Operand(SeqOneByteString::kHeaderSize - kHeapObjectTag));
  __ add(left, left, Operand(scratch1));
  __ add(right, right, Operand(scratch1));
  __ rsb(length, length, Operand::Zero());
  Register index = length;  // index = -length;

  // Compare loop.
  Label loop;
  __ bind(&loop);
  __ ldrb(scratch1, MemOperand(left, index));
  __ ldrb(scratch2, MemOperand(right, index));
  __ cmp(scratch1, scratch2);
  __ b(ne, chars_not_equal);
  __ add(index, index, Operand(1), SetCC);
  __ b(ne, &loop);
}


void StringCompareStub::Generate(MacroAssembler* masm) {
  Label runtime;

  Counters* counters = isolate()->counters();

  // Stack frame on entry.
  //  sp[0]: right string
  //  sp[4]: left string
  __ Ldrd(r0 , r1, MemOperand(sp));  // Load right in r0, left in r1.

  Label not_same;
  __ cmp(r0, r1);
  __ b(ne, &not_same);
  STATIC_ASSERT(EQUAL == 0);
  STATIC_ASSERT(kSmiTag == 0);
  __ mov(r0, Operand(Smi::FromInt(EQUAL)));
  __ IncrementCounter(counters->string_compare_native(), 1, r1, r2);
  __ add(sp, sp, Operand(2 * kPointerSize));
  __ Ret();

  __ bind(&not_same);

  // Check that both objects are sequential one-byte strings.
  __ JumpIfNotBothSequentialOneByteStrings(r1, r0, r2, r3, &runtime);

  // Compare flat one-byte strings natively. Remove arguments from stack first.
  __ IncrementCounter(counters->string_compare_native(), 1, r2, r3);
  __ add(sp, sp, Operand(2 * kPointerSize));
  StringHelper::GenerateCompareFlatOneByteStrings(masm, r1, r0, r2, r3, r4, r5);

  // Call the runtime; it returns -1 (less), 0 (equal), or 1 (greater)
  // tagged as a small integer.
  __ bind(&runtime);
  __ TailCallRuntime(Runtime::kStringCompare, 2, 1);
}


void BinaryOpICWithAllocationSiteStub::Generate(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- r1    : left
  //  -- r0    : right
  //  -- lr    : return address
  // -----------------------------------

  // Load r2 with the allocation site.  We stick an undefined dummy value here
  // and replace it with the real allocation site later when we instantiate this
  // stub in BinaryOpICWithAllocationSiteStub::GetCodeCopyFromTemplate().
  __ Move(r2, handle(isolate()->heap()->undefined_value()));

  // Make sure that we actually patched the allocation site.
  if (FLAG_debug_code) {
    __ tst(r2, Operand(kSmiTagMask));
    __ Assert(ne, kExpectedAllocationSite);
    __ push(r2);
    __ ldr(r2, FieldMemOperand(r2, HeapObject::kMapOffset));
    __ LoadRoot(ip, Heap::kAllocationSiteMapRootIndex);
    __ cmp(r2, ip);
    __ pop(r2);
    __ Assert(eq, kExpectedAllocationSite);
  }

  // Tail call into the stub that handles binary operations with allocation
  // sites.
  BinaryOpWithAllocationSiteStub stub(isolate(), state());
  __ TailCallStub(&stub);
}


void CompareICStub::GenerateSmis(MacroAssembler* masm) {
  DCHECK(state() == CompareICState::SMI);
  Label miss;
  __ orr(r2, r1, r0);
  __ JumpIfNotSmi(r2, &miss);

  if (GetCondition() == eq) {
    // For equality we do not care about the sign of the result.
    __ sub(r0, r0, r1, SetCC);
  } else {
    // Untag before subtracting to avoid handling overflow.
    __ SmiUntag(r1);
    __ sub(r0, r1, Operand::SmiUntag(r0));
  }
  __ Ret();

  __ bind(&miss);
  GenerateMiss(masm);
}


void CompareICStub::GenerateNumbers(MacroAssembler* masm) {
  DCHECK(state() == CompareICState::NUMBER);

  Label generic_stub;
  Label unordered, maybe_undefined1, maybe_undefined2;
  Label miss;

  if (left() == CompareICState::SMI) {
    __ JumpIfNotSmi(r1, &miss);
  }
  if (right() == CompareICState::SMI) {
    __ JumpIfNotSmi(r0, &miss);
  }

  // Inlining the double comparison and falling back to the general compare
  // stub if NaN is involved.
  // Load left and right operand.
  Label done, left, left_smi, right_smi;
  __ JumpIfSmi(r0, &right_smi);
  __ CheckMap(r0, r2, Heap::kHeapNumberMapRootIndex, &maybe_undefined1,
              DONT_DO_SMI_CHECK);
  __ sub(r2, r0, Operand(kHeapObjectTag));
  __ vldr(d1, r2, HeapNumber::kValueOffset);
  __ b(&left);
  __ bind(&right_smi);
  __ SmiToDouble(d1, r0);

  __ bind(&left);
  __ JumpIfSmi(r1, &left_smi);
  __ CheckMap(r1, r2, Heap::kHeapNumberMapRootIndex, &maybe_undefined2,
              DONT_DO_SMI_CHECK);
  __ sub(r2, r1, Operand(kHeapObjectTag));
  __ vldr(d0, r2, HeapNumber::kValueOffset);
  __ b(&done);
  __ bind(&left_smi);
  __ SmiToDouble(d0, r1);

  __ bind(&done);
  // Compare operands.
  __ VFPCompareAndSetFlags(d0, d1);

  // Don't base result on status bits when a NaN is involved.
  __ b(vs, &unordered);

  // Return a result of -1, 0, or 1, based on status bits.
  __ mov(r0, Operand(EQUAL), LeaveCC, eq);
  __ mov(r0, Operand(LESS), LeaveCC, lt);
  __ mov(r0, Operand(GREATER), LeaveCC, gt);
  __ Ret();

  __ bind(&unordered);
  __ bind(&generic_stub);
  CompareICStub stub(isolate(), op(), CompareICState::GENERIC,
                     CompareICState::GENERIC, CompareICState::GENERIC);
  __ Jump(stub.GetCode(), RelocInfo::CODE_TARGET);

  __ bind(&maybe_undefined1);
  if (Token::IsOrderedRelationalCompareOp(op())) {
    __ CompareRoot(r0, Heap::kUndefinedValueRootIndex);
    __ b(ne, &miss);
    __ JumpIfSmi(r1, &unordered);
    __ CompareObjectType(r1, r2, r2, HEAP_NUMBER_TYPE);
    __ b(ne, &maybe_undefined2);
    __ jmp(&unordered);
  }

  __ bind(&maybe_undefined2);
  if (Token::IsOrderedRelationalCompareOp(op())) {
    __ CompareRoot(r1, Heap::kUndefinedValueRootIndex);
    __ b(eq, &unordered);
  }

  __ bind(&miss);
  GenerateMiss(masm);
}


void CompareICStub::GenerateInternalizedStrings(MacroAssembler* masm) {
  DCHECK(state() == CompareICState::INTERNALIZED_STRING);
  Label miss;

  // Registers containing left and right operands respectively.
  Register left = r1;
  Register right = r0;
  Register tmp1 = r2;
  Register tmp2 = r3;

  // Check that both operands are heap objects.
  __ JumpIfEitherSmi(left, right, &miss);

  // Check that both operands are internalized strings.
  __ ldr(tmp1, FieldMemOperand(left, HeapObject::kMapOffset));
  __ ldr(tmp2, FieldMemOperand(right, HeapObject::kMapOffset));
  __ ldrb(tmp1, FieldMemOperand(tmp1, Map::kInstanceTypeOffset));
  __ ldrb(tmp2, FieldMemOperand(tmp2, Map::kInstanceTypeOffset));
  STATIC_ASSERT(kInternalizedTag == 0 && kStringTag == 0);
  __ orr(tmp1, tmp1, Operand(tmp2));
  __ tst(tmp1, Operand(kIsNotStringMask | kIsNotInternalizedMask));
  __ b(ne, &miss);

  // Internalized strings are compared by identity.
  __ cmp(left, right);
  // Make sure r0 is non-zero. At this point input operands are
  // guaranteed to be non-zero.
  DCHECK(right.is(r0));
  STATIC_ASSERT(EQUAL == 0);
  STATIC_ASSERT(kSmiTag == 0);
  __ mov(r0, Operand(Smi::FromInt(EQUAL)), LeaveCC, eq);
  __ Ret();

  __ bind(&miss);
  GenerateMiss(masm);
}


void CompareICStub::GenerateUniqueNames(MacroAssembler* masm) {
  DCHECK(state() == CompareICState::UNIQUE_NAME);
  DCHECK(GetCondition() == eq);
  Label miss;

  // Registers containing left and right operands respectively.
  Register left = r1;
  Register right = r0;
  Register tmp1 = r2;
  Register tmp2 = r3;

  // Check that both operands are heap objects.
  __ JumpIfEitherSmi(left, right, &miss);

  // Check that both operands are unique names. This leaves the instance
  // types loaded in tmp1 and tmp2.
  __ ldr(tmp1, FieldMemOperand(left, HeapObject::kMapOffset));
  __ ldr(tmp2, FieldMemOperand(right, HeapObject::kMapOffset));
  __ ldrb(tmp1, FieldMemOperand(tmp1, Map::kInstanceTypeOffset));
  __ ldrb(tmp2, FieldMemOperand(tmp2, Map::kInstanceTypeOffset));

  __ JumpIfNotUniqueNameInstanceType(tmp1, &miss);
  __ JumpIfNotUniqueNameInstanceType(tmp2, &miss);

  // Unique names are compared by identity.
  __ cmp(left, right);
  // Make sure r0 is non-zero. At this point input operands are
  // guaranteed to be non-zero.
  DCHECK(right.is(r0));
  STATIC_ASSERT(EQUAL == 0);
  STATIC_ASSERT(kSmiTag == 0);
  __ mov(r0, Operand(Smi::FromInt(EQUAL)), LeaveCC, eq);
  __ Ret();

  __ bind(&miss);
  GenerateMiss(masm);
}


void CompareICStub::GenerateStrings(MacroAssembler* masm) {
  DCHECK(state() == CompareICState::STRING);
  Label miss;

  bool equality = Token::IsEqualityOp(op());

  // Registers containing left and right operands respectively.
  Register left = r1;
  Register right = r0;
  Register tmp1 = r2;
  Register tmp2 = r3;
  Register tmp3 = r4;
  Register tmp4 = r5;

  // Check that both operands are heap objects.
  __ JumpIfEitherSmi(left, right, &miss);

  // Check that both operands are strings. This leaves the instance
  // types loaded in tmp1 and tmp2.
  __ ldr(tmp1, FieldMemOperand(left, HeapObject::kMapOffset));
  __ ldr(tmp2, FieldMemOperand(right, HeapObject::kMapOffset));
  __ ldrb(tmp1, FieldMemOperand(tmp1, Map::kInstanceTypeOffset));
  __ ldrb(tmp2, FieldMemOperand(tmp2, Map::kInstanceTypeOffset));
  STATIC_ASSERT(kNotStringTag != 0);
  __ orr(tmp3, tmp1, tmp2);
  __ tst(tmp3, Operand(kIsNotStringMask));
  __ b(ne, &miss);

  // Fast check for identical strings.
  __ cmp(left, right);
  STATIC_ASSERT(EQUAL == 0);
  STATIC_ASSERT(kSmiTag == 0);
  __ mov(r0, Operand(Smi::FromInt(EQUAL)), LeaveCC, eq);
  __ Ret(eq);

  // Handle not identical strings.

  // Check that both strings are internalized strings. If they are, we're done
  // because we already know they are not identical. We know they are both
  // strings.
  if (equality) {
    DCHECK(GetCondition() == eq);
    STATIC_ASSERT(kInternalizedTag == 0);
    __ orr(tmp3, tmp1, Operand(tmp2));
    __ tst(tmp3, Operand(kIsNotInternalizedMask));
    // Make sure r0 is non-zero. At this point input operands are
    // guaranteed to be non-zero.
    DCHECK(right.is(r0));
    __ Ret(eq);
  }

  // Check that both strings are sequential one-byte.
  Label runtime;
  __ JumpIfBothInstanceTypesAreNotSequentialOneByte(tmp1, tmp2, tmp3, tmp4,
                                                    &runtime);

  // Compare flat one-byte strings. Returns when done.
  if (equality) {
    StringHelper::GenerateFlatOneByteStringEquals(masm, left, right, tmp1, tmp2,
                                                  tmp3);
  } else {
    StringHelper::GenerateCompareFlatOneByteStrings(masm, left, right, tmp1,
                                                    tmp2, tmp3, tmp4);
  }

  // Handle more complex cases in runtime.
  __ bind(&runtime);
  __ Push(left, right);
  if (equality) {
    __ TailCallRuntime(Runtime::kStringEquals, 2, 1);
  } else {
    __ TailCallRuntime(Runtime::kStringCompare, 2, 1);
  }

  __ bind(&miss);
  GenerateMiss(masm);
}


void CompareICStub::GenerateObjects(MacroAssembler* masm) {
  DCHECK(state() == CompareICState::OBJECT);
  Label miss;
  __ and_(r2, r1, Operand(r0));
  __ JumpIfSmi(r2, &miss);

  __ CompareObjectType(r0, r2, r2, JS_OBJECT_TYPE);
  __ b(ne, &miss);
  __ CompareObjectType(r1, r2, r2, JS_OBJECT_TYPE);
  __ b(ne, &miss);

  DCHECK(GetCondition() == eq);
  __ sub(r0, r0, Operand(r1));
  __ Ret();

  __ bind(&miss);
  GenerateMiss(masm);
}


void CompareICStub::GenerateKnownObjects(MacroAssembler* masm) {
  Label miss;
  __ and_(r2, r1, Operand(r0));
  __ JumpIfSmi(r2, &miss);
  __ ldr(r2, FieldMemOperand(r0, HeapObject::kMapOffset));
  __ ldr(r3, FieldMemOperand(r1, HeapObject::kMapOffset));
  __ cmp(r2, Operand(known_map_));
  __ b(ne, &miss);
  __ cmp(r3, Operand(known_map_));
  __ b(ne, &miss);

  __ sub(r0, r0, Operand(r1));
  __ Ret();

  __ bind(&miss);
  GenerateMiss(masm);
}


void CompareICStub::GenerateMiss(MacroAssembler* masm) {
  {
    // Call the runtime system in a fresh internal frame.
    ExternalReference miss =
        ExternalReference(IC_Utility(IC::kCompareIC_Miss), isolate());

    FrameAndConstantPoolScope scope(masm, StackFrame::INTERNAL);
    __ Push(r1, r0);
    __ Push(lr, r1, r0);
    __ mov(ip, Operand(Smi::FromInt(op())));
    __ push(ip);
    __ CallExternalReference(miss, 3);
    // Compute the entry point of the rewritten stub.
    __ add(r2, r0, Operand(Code::kHeaderSize - kHeapObjectTag));
    // Restore registers.
    __ pop(lr);
    __ Pop(r1, r0);
  }

  __ Jump(r2);
}


void DirectCEntryStub::Generate(MacroAssembler* masm) {
  // Place the return address on the stack, making the call
  // GC safe. The RegExp backend also relies on this.
  __ str(lr, MemOperand(sp, 0));
  __ blx(ip);  // Call the C++ function.
  __ VFPEnsureFPSCRState(r2);
  __ ldr(pc, MemOperand(sp, 0));
}


void DirectCEntryStub::GenerateCall(MacroAssembler* masm,
                                    Register target) {
  intptr_t code =
      reinterpret_cast<intptr_t>(GetCode().location());
  __ Move(ip, target);
  __ mov(lr, Operand(code, RelocInfo::CODE_TARGET));
  __ blx(lr);  // Call the stub.
}


void NameDictionaryLookupStub::GenerateNegativeLookup(MacroAssembler* masm,
                                                      Label* miss,
                                                      Label* done,
                                                      Register receiver,
                                                      Register properties,
                                                      Handle<Name> name,
                                                      Register scratch0) {
  DCHECK(name->IsUniqueName());
  // If names of slots in range from 1 to kProbes - 1 for the hash value are
  // not equal to the name and kProbes-th slot is not used (its name is the
  // undefined value), it guarantees the hash table doesn't contain the
  // property. It's true even if some slots represent deleted properties
  // (their names are the hole value).
  for (int i = 0; i < kInlinedProbes; i++) {
    // scratch0 points to properties hash.
    // Compute the masked index: (hash + i + i * i) & mask.
    Register index = scratch0;
    // Capacity is smi 2^n.
    __ ldr(index, FieldMemOperand(properties, kCapacityOffset));
    __ sub(index, index, Operand(1));
    __ and_(index, index, Operand(
        Smi::FromInt(name->Hash() + NameDictionary::GetProbeOffset(i))));

    // Scale the index by multiplying by the entry size.
    DCHECK(NameDictionary::kEntrySize == 3);
    __ add(index, index, Operand(index, LSL, 1));  // index *= 3.

    Register entity_name = scratch0;
    // Having undefined at this place means the name is not contained.
    DCHECK_EQ(kSmiTagSize, 1);
    Register tmp = properties;
    __ add(tmp, properties, Operand(index, LSL, 1));
    __ ldr(entity_name, FieldMemOperand(tmp, kElementsStartOffset));

    DCHECK(!tmp.is(entity_name));
    __ LoadRoot(tmp, Heap::kUndefinedValueRootIndex);
    __ cmp(entity_name, tmp);
    __ b(eq, done);

    // Load the hole ready for use below:
    __ LoadRoot(tmp, Heap::kTheHoleValueRootIndex);

    // Stop if found the property.
    __ cmp(entity_name, Operand(Handle<Name>(name)));
    __ b(eq, miss);

    Label good;
    __ cmp(entity_name, tmp);
    __ b(eq, &good);

    // Check if the entry name is not a unique name.
    __ ldr(entity_name, FieldMemOperand(entity_name, HeapObject::kMapOffset));
    __ ldrb(entity_name,
            FieldMemOperand(entity_name, Map::kInstanceTypeOffset));
    __ JumpIfNotUniqueNameInstanceType(entity_name, miss);
    __ bind(&good);

    // Restore the properties.
    __ ldr(properties,
           FieldMemOperand(receiver, JSObject::kPropertiesOffset));
  }

  const int spill_mask =
      (lr.bit() | r6.bit() | r5.bit() | r4.bit() | r3.bit() |
       r2.bit() | r1.bit() | r0.bit());

  __ stm(db_w, sp, spill_mask);
  __ ldr(r0, FieldMemOperand(receiver, JSObject::kPropertiesOffset));
  __ mov(r1, Operand(Handle<Name>(name)));
  NameDictionaryLookupStub stub(masm->isolate(), NEGATIVE_LOOKUP);
  __ CallStub(&stub);
  __ cmp(r0, Operand::Zero());
  __ ldm(ia_w, sp, spill_mask);

  __ b(eq, done);
  __ b(ne, miss);
}


// Probe the name dictionary in the |elements| register. Jump to the
// |done| label if a property with the given name is found. Jump to
// the |miss| label otherwise.
// If lookup was successful |scratch2| will be equal to elements + 4 * index.
void NameDictionaryLookupStub::GeneratePositiveLookup(MacroAssembler* masm,
                                                      Label* miss,
                                                      Label* done,
                                                      Register elements,
                                                      Register name,
                                                      Register scratch1,
                                                      Register scratch2) {
  DCHECK(!elements.is(scratch1));
  DCHECK(!elements.is(scratch2));
  DCHECK(!name.is(scratch1));
  DCHECK(!name.is(scratch2));

  __ AssertName(name);

  // Compute the capacity mask.
  __ ldr(scratch1, FieldMemOperand(elements, kCapacityOffset));
  __ SmiUntag(scratch1);
  __ sub(scratch1, scratch1, Operand(1));

  // Generate an unrolled loop that performs a few probes before
  // giving up. Measurements done on Gmail indicate that 2 probes
  // cover ~93% of loads from dictionaries.
  for (int i = 0; i < kInlinedProbes; i++) {
    // Compute the masked index: (hash + i + i * i) & mask.
    __ ldr(scratch2, FieldMemOperand(name, Name::kHashFieldOffset));
    if (i > 0) {
      // Add the probe offset (i + i * i) left shifted to avoid right shifting
      // the hash in a separate instruction. The value hash + i + i * i is right
      // shifted in the following and instruction.
      DCHECK(NameDictionary::GetProbeOffset(i) <
             1 << (32 - Name::kHashFieldOffset));
      __ add(scratch2, scratch2, Operand(
          NameDictionary::GetProbeOffset(i) << Name::kHashShift));
    }
    __ and_(scratch2, scratch1, Operand(scratch2, LSR, Name::kHashShift));

    // Scale the index by multiplying by the element size.
    DCHECK(NameDictionary::kEntrySize == 3);
    // scratch2 = scratch2 * 3.
    __ add(scratch2, scratch2, Operand(scratch2, LSL, 1));

    // Check if the key is identical to the name.
    __ add(scratch2, elements, Operand(scratch2, LSL, 2));
    __ ldr(ip, FieldMemOperand(scratch2, kElementsStartOffset));
    __ cmp(name, Operand(ip));
    __ b(eq, done);
  }

  const int spill_mask =
      (lr.bit() | r6.bit() | r5.bit() | r4.bit() |
       r3.bit() | r2.bit() | r1.bit() | r0.bit()) &
      ~(scratch1.bit() | scratch2.bit());

  __ stm(db_w, sp, spill_mask);
  if (name.is(r0)) {
    DCHECK(!elements.is(r1));
    __ Move(r1, name);
    __ Move(r0, elements);
  } else {
    __ Move(r0, elements);
    __ Move(r1, name);
  }
  NameDictionaryLookupStub stub(masm->isolate(), POSITIVE_LOOKUP);
  __ CallStub(&stub);
  __ cmp(r0, Operand::Zero());
  __ mov(scratch2, Operand(r2));
  __ ldm(ia_w, sp, spill_mask);

  __ b(ne, done);
  __ b(eq, miss);
}


void NameDictionaryLookupStub::Generate(MacroAssembler* masm) {
  // This stub overrides SometimesSetsUpAFrame() to return false.  That means
  // we cannot call anything that could cause a GC from this stub.
  // Registers:
  //  result: NameDictionary to probe
  //  r1: key
  //  dictionary: NameDictionary to probe.
  //  index: will hold an index of entry if lookup is successful.
  //         might alias with result_.
  // Returns:
  //  result_ is zero if lookup failed, non zero otherwise.

  Register result = r0;
  Register dictionary = r0;
  Register key = r1;
  Register index = r2;
  Register mask = r3;
  Register hash = r4;
  Register undefined = r5;
  Register entry_key = r6;

  Label in_dictionary, maybe_in_dictionary, not_in_dictionary;

  __ ldr(mask, FieldMemOperand(dictionary, kCapacityOffset));
  __ SmiUntag(mask);
  __ sub(mask, mask, Operand(1));

  __ ldr(hash, FieldMemOperand(key, Name::kHashFieldOffset));

  __ LoadRoot(undefined, Heap::kUndefinedValueRootIndex);

  for (int i = kInlinedProbes; i < kTotalProbes; i++) {
    // Compute the masked index: (hash + i + i * i) & mask.
    // Capacity is smi 2^n.
    if (i > 0) {
      // Add the probe offset (i + i * i) left shifted to avoid right shifting
      // the hash in a separate instruction. The value hash + i + i * i is right
      // shifted in the following and instruction.
      DCHECK(NameDictionary::GetProbeOffset(i) <
             1 << (32 - Name::kHashFieldOffset));
      __ add(index, hash, Operand(
          NameDictionary::GetProbeOffset(i) << Name::kHashShift));
    } else {
      __ mov(index, Operand(hash));
    }
    __ and_(index, mask, Operand(index, LSR, Name::kHashShift));

    // Scale the index by multiplying by the entry size.
    DCHECK(NameDictionary::kEntrySize == 3);
    __ add(index, index, Operand(index, LSL, 1));  // index *= 3.

    DCHECK_EQ(kSmiTagSize, 1);
    __ add(index, dictionary, Operand(index, LSL, 2));
    __ ldr(entry_key, FieldMemOperand(index, kElementsStartOffset));

    // Having undefined at this place means the name is not contained.
    __ cmp(entry_key, Operand(undefined));
    __ b(eq, &not_in_dictionary);

    // Stop if found the property.
    __ cmp(entry_key, Operand(key));
    __ b(eq, &in_dictionary);

    if (i != kTotalProbes - 1 && mode() == NEGATIVE_LOOKUP) {
      // Check if the entry name is not a unique name.
      __ ldr(entry_key, FieldMemOperand(entry_key, HeapObject::kMapOffset));
      __ ldrb(entry_key,
              FieldMemOperand(entry_key, Map::kInstanceTypeOffset));
      __ JumpIfNotUniqueNameInstanceType(entry_key, &maybe_in_dictionary);
    }
  }

  __ bind(&maybe_in_dictionary);
  // If we are doing negative lookup then probing failure should be
  // treated as a lookup success. For positive lookup probing failure
  // should be treated as lookup failure.
  if (mode() == POSITIVE_LOOKUP) {
    __ mov(result, Operand::Zero());
    __ Ret();
  }

  __ bind(&in_dictionary);
  __ mov(result, Operand(1));
  __ Ret();

  __ bind(&not_in_dictionary);
  __ mov(result, Operand::Zero());
  __ Ret();
}


void StoreBufferOverflowStub::GenerateFixedRegStubsAheadOfTime(
    Isolate* isolate) {
  StoreBufferOverflowStub stub1(isolate, kDontSaveFPRegs);
  stub1.GetCode();
  // Hydrogen code stubs need stub2 at snapshot time.
  StoreBufferOverflowStub stub2(isolate, kSaveFPRegs);
  stub2.GetCode();
}


// Takes the input in 3 registers: address_ value_ and object_.  A pointer to
// the value has just been written into the object, now this stub makes sure
// we keep the GC informed.  The word in the object where the value has been
// written is in the address register.
void RecordWriteStub::Generate(MacroAssembler* masm) {
  Label skip_to_incremental_noncompacting;
  Label skip_to_incremental_compacting;

  // The first two instructions are generated with labels so as to get the
  // offset fixed up correctly by the bind(Label*) call.  We patch it back and
  // forth between a compare instructions (a nop in this position) and the
  // real branch when we start and stop incremental heap marking.
  // See RecordWriteStub::Patch for details.
  {
    // Block literal pool emission, as the position of these two instructions
    // is assumed by the patching code.
    Assembler::BlockConstPoolScope block_const_pool(masm);
    __ b(&skip_to_incremental_noncompacting);
    __ b(&skip_to_incremental_compacting);
  }

  if (remembered_set_action() == EMIT_REMEMBERED_SET) {
    __ RememberedSetHelper(object(), address(), value(), save_fp_regs_mode(),
                           MacroAssembler::kReturnAtEnd);
  }
  __ Ret();

  __ bind(&skip_to_incremental_noncompacting);
  GenerateIncremental(masm, INCREMENTAL);

  __ bind(&skip_to_incremental_compacting);
  GenerateIncremental(masm, INCREMENTAL_COMPACTION);

  // Initial mode of the stub is expected to be STORE_BUFFER_ONLY.
  // Will be checked in IncrementalMarking::ActivateGeneratedStub.
  DCHECK(Assembler::GetBranchOffset(masm->instr_at(0)) < (1 << 12));
  DCHECK(Assembler::GetBranchOffset(masm->instr_at(4)) < (1 << 12));
  PatchBranchIntoNop(masm, 0);
  PatchBranchIntoNop(masm, Assembler::kInstrSize);
}


void RecordWriteStub::GenerateIncremental(MacroAssembler* masm, Mode mode) {
  regs_.Save(masm);

  if (remembered_set_action() == EMIT_REMEMBERED_SET) {
    Label dont_need_remembered_set;

    __ ldr(regs_.scratch0(), MemOperand(regs_.address(), 0));
    __ JumpIfNotInNewSpace(regs_.scratch0(),  // Value.
                           regs_.scratch0(),
                           &dont_need_remembered_set);

    __ CheckPageFlag(regs_.object(),
                     regs_.scratch0(),
                     1 << MemoryChunk::SCAN_ON_SCAVENGE,
                     ne,
                     &dont_need_remembered_set);

    // First notify the incremental marker if necessary, then update the
    // remembered set.
    CheckNeedsToInformIncrementalMarker(
        masm, kUpdateRememberedSetOnNoNeedToInformIncrementalMarker, mode);
    InformIncrementalMarker(masm);
    regs_.Restore(masm);
    __ RememberedSetHelper(object(), address(), value(), save_fp_regs_mode(),
                           MacroAssembler::kReturnAtEnd);

    __ bind(&dont_need_remembered_set);
  }

  CheckNeedsToInformIncrementalMarker(
      masm, kReturnOnNoNeedToInformIncrementalMarker, mode);
  InformIncrementalMarker(masm);
  regs_.Restore(masm);
  __ Ret();
}


void RecordWriteStub::InformIncrementalMarker(MacroAssembler* masm) {
  regs_.SaveCallerSaveRegisters(masm, save_fp_regs_mode());
  int argument_count = 3;
  __ PrepareCallCFunction(argument_count, regs_.scratch0());
  Register address =
      r0.is(regs_.address()) ? regs_.scratch0() : regs_.address();
  DCHECK(!address.is(regs_.object()));
  DCHECK(!address.is(r0));
  __ Move(address, regs_.address());
  __ Move(r0, regs_.object());
  __ Move(r1, address);
  __ mov(r2, Operand(ExternalReference::isolate_address(isolate())));

  AllowExternalCallThatCantCauseGC scope(masm);
  __ CallCFunction(
      ExternalReference::incremental_marking_record_write_function(isolate()),
      argument_count);
  regs_.RestoreCallerSaveRegisters(masm, save_fp_regs_mode());
}


void RecordWriteStub::CheckNeedsToInformIncrementalMarker(
    MacroAssembler* masm,
    OnNoNeedToInformIncrementalMarker on_no_need,
    Mode mode) {
  Label on_black;
  Label need_incremental;
  Label need_incremental_pop_scratch;

  __ and_(regs_.scratch0(), regs_.object(), Operand(~Page::kPageAlignmentMask));
  __ ldr(regs_.scratch1(),
         MemOperand(regs_.scratch0(),
                    MemoryChunk::kWriteBarrierCounterOffset));
  __ sub(regs_.scratch1(), regs_.scratch1(), Operand(1), SetCC);
  __ str(regs_.scratch1(),
         MemOperand(regs_.scratch0(),
                    MemoryChunk::kWriteBarrierCounterOffset));
  __ b(mi, &need_incremental);

  // Let's look at the color of the object:  If it is not black we don't have
  // to inform the incremental marker.
  __ JumpIfBlack(regs_.object(), regs_.scratch0(), regs_.scratch1(), &on_black);

  regs_.Restore(masm);
  if (on_no_need == kUpdateRememberedSetOnNoNeedToInformIncrementalMarker) {
    __ RememberedSetHelper(object(), address(), value(), save_fp_regs_mode(),
                           MacroAssembler::kReturnAtEnd);
  } else {
    __ Ret();
  }

  __ bind(&on_black);

  // Get the value from the slot.
  __ ldr(regs_.scratch0(), MemOperand(regs_.address(), 0));

  if (mode == INCREMENTAL_COMPACTION) {
    Label ensure_not_white;

    __ CheckPageFlag(regs_.scratch0(),  // Contains value.
                     regs_.scratch1(),  // Scratch.
                     MemoryChunk::kEvacuationCandidateMask,
                     eq,
                     &ensure_not_white);

    __ CheckPageFlag(regs_.object(),
                     regs_.scratch1(),  // Scratch.
                     MemoryChunk::kSkipEvacuationSlotsRecordingMask,
                     eq,
                     &need_incremental);

    __ bind(&ensure_not_white);
  }

  // We need extra registers for this, so we push the object and the address
  // register temporarily.
  __ Push(regs_.object(), regs_.address());
  __ EnsureNotWhite(regs_.scratch0(),  // The value.
                    regs_.scratch1(),  // Scratch.
                    regs_.object(),  // Scratch.
                    regs_.address(),  // Scratch.
                    &need_incremental_pop_scratch);
  __ Pop(regs_.object(), regs_.address());

  regs_.Restore(masm);
  if (on_no_need == kUpdateRememberedSetOnNoNeedToInformIncrementalMarker) {
    __ RememberedSetHelper(object(), address(), value(), save_fp_regs_mode(),
                           MacroAssembler::kReturnAtEnd);
  } else {
    __ Ret();
  }

  __ bind(&need_incremental_pop_scratch);
  __ Pop(regs_.object(), regs_.address());

  __ bind(&need_incremental);

  // Fall through when we need to inform the incremental marker.
}


void StoreArrayLiteralElementStub::Generate(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- r0    : element value to store
  //  -- r3    : element index as smi
  //  -- sp[0] : array literal index in function as smi
  //  -- sp[4] : array literal
  // clobbers r1, r2, r4
  // -----------------------------------

  Label element_done;
  Label double_elements;
  Label smi_element;
  Label slow_elements;
  Label fast_elements;

  // Get array literal index, array literal and its map.
  __ ldr(r4, MemOperand(sp, 0 * kPointerSize));
  __ ldr(r1, MemOperand(sp, 1 * kPointerSize));
  __ ldr(r2, FieldMemOperand(r1, JSObject::kMapOffset));

  __ CheckFastElements(r2, r5, &double_elements);
  // FAST_*_SMI_ELEMENTS or FAST_*_ELEMENTS
  __ JumpIfSmi(r0, &smi_element);
  __ CheckFastSmiElements(r2, r5, &fast_elements);

  // Store into the array literal requires a elements transition. Call into
  // the runtime.
  __ bind(&slow_elements);
  // call.
  __ Push(r1, r3, r0);
  __ ldr(r5, MemOperand(fp, JavaScriptFrameConstants::kFunctionOffset));
  __ ldr(r5, FieldMemOperand(r5, JSFunction::kLiteralsOffset));
  __ Push(r5, r4);
  __ TailCallRuntime(Runtime::kStoreArrayLiteralElement, 5, 1);

  // Array literal has ElementsKind of FAST_*_ELEMENTS and value is an object.
  __ bind(&fast_elements);
  __ ldr(r5, FieldMemOperand(r1, JSObject::kElementsOffset));
  __ add(r6, r5, Operand::PointerOffsetFromSmiKey(r3));
  __ add(r6, r6, Operand(FixedArray::kHeaderSize - kHeapObjectTag));
  __ str(r0, MemOperand(r6, 0));
  // Update the write barrier for the array store.
  __ RecordWrite(r5, r6, r0, kLRHasNotBeenSaved, kDontSaveFPRegs,
                 EMIT_REMEMBERED_SET, OMIT_SMI_CHECK);
  __ Ret();

  // Array literal has ElementsKind of FAST_*_SMI_ELEMENTS or FAST_*_ELEMENTS,
  // and value is Smi.
  __ bind(&smi_element);
  __ ldr(r5, FieldMemOperand(r1, JSObject::kElementsOffset));
  __ add(r6, r5, Operand::PointerOffsetFromSmiKey(r3));
  __ str(r0, FieldMemOperand(r6, FixedArray::kHeaderSize));
  __ Ret();

  // Array literal has ElementsKind of FAST_DOUBLE_ELEMENTS.
  __ bind(&double_elements);
  __ ldr(r5, FieldMemOperand(r1, JSObject::kElementsOffset));
  __ StoreNumberToDoubleElements(r0, r3, r5, r6, d0, &slow_elements);
  __ Ret();
}


void StubFailureTrampolineStub::Generate(MacroAssembler* masm) {
  CEntryStub ces(isolate(), 1, kSaveFPRegs);
  __ Call(ces.GetCode(), RelocInfo::CODE_TARGET);
  int parameter_count_offset =
      StubFailureTrampolineFrame::kCallerStackParameterCountFrameOffset;
  __ ldr(r1, MemOperand(fp, parameter_count_offset));
  if (function_mode() == JS_FUNCTION_STUB_MODE) {
    __ add(r1, r1, Operand(1));
  }
  masm->LeaveFrame(StackFrame::STUB_FAILURE_TRAMPOLINE);
  __ mov(r1, Operand(r1, LSL, kPointerSizeLog2));
  __ add(sp, sp, r1);
  __ Ret();
}


void LoadICTrampolineStub::Generate(MacroAssembler* masm) {
  EmitLoadTypeFeedbackVector(masm, VectorLoadICDescriptor::VectorRegister());
  VectorLoadStub stub(isolate(), state());
  __ Jump(stub.GetCode(), RelocInfo::CODE_TARGET);
}


void KeyedLoadICTrampolineStub::Generate(MacroAssembler* masm) {
  EmitLoadTypeFeedbackVector(masm, VectorLoadICDescriptor::VectorRegister());
  VectorKeyedLoadStub stub(isolate());
  __ Jump(stub.GetCode(), RelocInfo::CODE_TARGET);
}


void ProfileEntryHookStub::MaybeCallEntryHook(MacroAssembler* masm) {
  if (masm->isolate()->function_entry_hook() != NULL) {
    ProfileEntryHookStub stub(masm->isolate());
    int code_size = masm->CallStubSize(&stub) + 2 * Assembler::kInstrSize;
    PredictableCodeSizeScope predictable(masm, code_size);
    __ push(lr);
    __ CallStub(&stub);
    __ pop(lr);
  }
}


void ProfileEntryHookStub::Generate(MacroAssembler* masm) {
  // The entry hook is a "push lr" instruction, followed by a call.
  const int32_t kReturnAddressDistanceFromFunctionStart =
      3 * Assembler::kInstrSize;

  // This should contain all kCallerSaved registers.
  const RegList kSavedRegs =
      1 <<  0 |  // r0
      1 <<  1 |  // r1
      1 <<  2 |  // r2
      1 <<  3 |  // r3
      1 <<  5 |  // r5
      1 <<  9;   // r9
  // We also save lr, so the count here is one higher than the mask indicates.
  const int32_t kNumSavedRegs = 7;

  DCHECK((kCallerSaved & kSavedRegs) == kCallerSaved);

  // Save all caller-save registers as this may be called from anywhere.
  __ stm(db_w, sp, kSavedRegs | lr.bit());

  // Compute the function's address for the first argument.
  __ sub(r0, lr, Operand(kReturnAddressDistanceFromFunctionStart));

  // The caller's return address is above the saved temporaries.
  // Grab that for the second argument to the hook.
  __ add(r1, sp, Operand(kNumSavedRegs * kPointerSize));

  // Align the stack if necessary.
  int frame_alignment = masm->ActivationFrameAlignment();
  if (frame_alignment > kPointerSize) {
    __ mov(r5, sp);
    DCHECK(base::bits::IsPowerOfTwo32(frame_alignment));
    __ and_(sp, sp, Operand(-frame_alignment));
  }

#if V8_HOST_ARCH_ARM
  int32_t entry_hook =
      reinterpret_cast<int32_t>(isolate()->function_entry_hook());
  __ mov(ip, Operand(entry_hook));
#else
  // Under the simulator we need to indirect the entry hook through a
  // trampoline function at a known address.
  // It additionally takes an isolate as a third parameter
  __ mov(r2, Operand(ExternalReference::isolate_address(isolate())));

  ApiFunction dispatcher(FUNCTION_ADDR(EntryHookTrampoline));
  __ mov(ip, Operand(ExternalReference(&dispatcher,
                                       ExternalReference::BUILTIN_CALL,
                                       isolate())));
#endif
  __ Call(ip);

  // Restore the stack pointer if needed.
  if (frame_alignment > kPointerSize) {
    __ mov(sp, r5);
  }

  // Also pop pc to get Ret(0).
  __ ldm(ia_w, sp, kSavedRegs | pc.bit());
}


template<class T>
static void CreateArrayDispatch(MacroAssembler* masm,
                                AllocationSiteOverrideMode mode) {
  if (mode == DISABLE_ALLOCATION_SITES) {
    T stub(masm->isolate(), GetInitialFastElementsKind(), mode);
    __ TailCallStub(&stub);
  } else if (mode == DONT_OVERRIDE) {
    int last_index = GetSequenceIndexFromFastElementsKind(
        TERMINAL_FAST_ELEMENTS_KIND);
    for (int i = 0; i <= last_index; ++i) {
      ElementsKind kind = GetFastElementsKindFromSequenceIndex(i);
      __ cmp(r3, Operand(kind));
      T stub(masm->isolate(), kind);
      __ TailCallStub(&stub, eq);
    }

    // If we reached this point there is a problem.
    __ Abort(kUnexpectedElementsKindInArrayConstructor);
  } else {
    UNREACHABLE();
  }
}


static void CreateArrayDispatchOneArgument(MacroAssembler* masm,
                                           AllocationSiteOverrideMode mode) {
  // r2 - allocation site (if mode != DISABLE_ALLOCATION_SITES)
  // r3 - kind (if mode != DISABLE_ALLOCATION_SITES)
  // r0 - number of arguments
  // r1 - constructor?
  // sp[0] - last argument
  Label normal_sequence;
  if (mode == DONT_OVERRIDE) {
    DCHECK(FAST_SMI_ELEMENTS == 0);
    DCHECK(FAST_HOLEY_SMI_ELEMENTS == 1);
    DCHECK(FAST_ELEMENTS == 2);
    DCHECK(FAST_HOLEY_ELEMENTS == 3);
    DCHECK(FAST_DOUBLE_ELEMENTS == 4);
    DCHECK(FAST_HOLEY_DOUBLE_ELEMENTS == 5);

    // is the low bit set? If so, we are holey and that is good.
    __ tst(r3, Operand(1));
    __ b(ne, &normal_sequence);
  }

  // look at the first argument
  __ ldr(r5, MemOperand(sp, 0));
  __ cmp(r5, Operand::Zero());
  __ b(eq, &normal_sequence);

  if (mode == DISABLE_ALLOCATION_SITES) {
    ElementsKind initial = GetInitialFastElementsKind();
    ElementsKind holey_initial = GetHoleyElementsKind(initial);

    ArraySingleArgumentConstructorStub stub_holey(masm->isolate(),
                                                  holey_initial,
                                                  DISABLE_ALLOCATION_SITES);
    __ TailCallStub(&stub_holey);

    __ bind(&normal_sequence);
    ArraySingleArgumentConstructorStub stub(masm->isolate(),
                                            initial,
                                            DISABLE_ALLOCATION_SITES);
    __ TailCallStub(&stub);
  } else if (mode == DONT_OVERRIDE) {
    // We are going to create a holey array, but our kind is non-holey.
    // Fix kind and retry (only if we have an allocation site in the slot).
    __ add(r3, r3, Operand(1));

    if (FLAG_debug_code) {
      __ ldr(r5, FieldMemOperand(r2, 0));
      __ CompareRoot(r5, Heap::kAllocationSiteMapRootIndex);
      __ Assert(eq, kExpectedAllocationSite);
    }

    // Save the resulting elements kind in type info. We can't just store r3
    // in the AllocationSite::transition_info field because elements kind is
    // restricted to a portion of the field...upper bits need to be left alone.
    STATIC_ASSERT(AllocationSite::ElementsKindBits::kShift == 0);
    __ ldr(r4, FieldMemOperand(r2, AllocationSite::kTransitionInfoOffset));
    __ add(r4, r4, Operand(Smi::FromInt(kFastElementsKindPackedToHoley)));
    __ str(r4, FieldMemOperand(r2, AllocationSite::kTransitionInfoOffset));

    __ bind(&normal_sequence);
    int last_index = GetSequenceIndexFromFastElementsKind(
        TERMINAL_FAST_ELEMENTS_KIND);
    for (int i = 0; i <= last_index; ++i) {
      ElementsKind kind = GetFastElementsKindFromSequenceIndex(i);
      __ cmp(r3, Operand(kind));
      ArraySingleArgumentConstructorStub stub(masm->isolate(), kind);
      __ TailCallStub(&stub, eq);
    }

    // If we reached this point there is a problem.
    __ Abort(kUnexpectedElementsKindInArrayConstructor);
  } else {
    UNREACHABLE();
  }
}


template<class T>
static void ArrayConstructorStubAheadOfTimeHelper(Isolate* isolate) {
  int to_index = GetSequenceIndexFromFastElementsKind(
      TERMINAL_FAST_ELEMENTS_KIND);
  for (int i = 0; i <= to_index; ++i) {
    ElementsKind kind = GetFastElementsKindFromSequenceIndex(i);
    T stub(isolate, kind);
    stub.GetCode();
    if (AllocationSite::GetMode(kind) != DONT_TRACK_ALLOCATION_SITE) {
      T stub1(isolate, kind, DISABLE_ALLOCATION_SITES);
      stub1.GetCode();
    }
  }
}


void ArrayConstructorStubBase::GenerateStubsAheadOfTime(Isolate* isolate) {
  ArrayConstructorStubAheadOfTimeHelper<ArrayNoArgumentConstructorStub>(
      isolate);
  ArrayConstructorStubAheadOfTimeHelper<ArraySingleArgumentConstructorStub>(
      isolate);
  ArrayConstructorStubAheadOfTimeHelper<ArrayNArgumentsConstructorStub>(
      isolate);
}


void InternalArrayConstructorStubBase::GenerateStubsAheadOfTime(
    Isolate* isolate) {
  ElementsKind kinds[2] = { FAST_ELEMENTS, FAST_HOLEY_ELEMENTS };
  for (int i = 0; i < 2; i++) {
    // For internal arrays we only need a few things
    InternalArrayNoArgumentConstructorStub stubh1(isolate, kinds[i]);
    stubh1.GetCode();
    InternalArraySingleArgumentConstructorStub stubh2(isolate, kinds[i]);
    stubh2.GetCode();
    InternalArrayNArgumentsConstructorStub stubh3(isolate, kinds[i]);
    stubh3.GetCode();
  }
}


void ArrayConstructorStub::GenerateDispatchToArrayStub(
    MacroAssembler* masm,
    AllocationSiteOverrideMode mode) {
  if (argument_count() == ANY) {
    Label not_zero_case, not_one_case;
    __ tst(r0, r0);
    __ b(ne, &not_zero_case);
    CreateArrayDispatch<ArrayNoArgumentConstructorStub>(masm, mode);

    __ bind(&not_zero_case);
    __ cmp(r0, Operand(1));
    __ b(gt, &not_one_case);
    CreateArrayDispatchOneArgument(masm, mode);

    __ bind(&not_one_case);
    CreateArrayDispatch<ArrayNArgumentsConstructorStub>(masm, mode);
  } else if (argument_count() == NONE) {
    CreateArrayDispatch<ArrayNoArgumentConstructorStub>(masm, mode);
  } else if (argument_count() == ONE) {
    CreateArrayDispatchOneArgument(masm, mode);
  } else if (argument_count() == MORE_THAN_ONE) {
    CreateArrayDispatch<ArrayNArgumentsConstructorStub>(masm, mode);
  } else {
    UNREACHABLE();
  }
}


void ArrayConstructorStub::Generate(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- r0 : argc (only if argument_count() == ANY)
  //  -- r1 : constructor
  //  -- r2 : AllocationSite or undefined
  //  -- sp[0] : return address
  //  -- sp[4] : last argument
  // -----------------------------------

  if (FLAG_debug_code) {
    // The array construct code is only set for the global and natives
    // builtin Array functions which always have maps.

    // Initial map for the builtin Array function should be a map.
    __ ldr(r4, FieldMemOperand(r1, JSFunction::kPrototypeOrInitialMapOffset));
    // Will both indicate a NULL and a Smi.
    __ tst(r4, Operand(kSmiTagMask));
    __ Assert(ne, kUnexpectedInitialMapForArrayFunction);
    __ CompareObjectType(r4, r4, r5, MAP_TYPE);
    __ Assert(eq, kUnexpectedInitialMapForArrayFunction);

    // We should either have undefined in r2 or a valid AllocationSite
    __ AssertUndefinedOrAllocationSite(r2, r4);
  }

  Label no_info;
  // Get the elements kind and case on that.
  __ CompareRoot(r2, Heap::kUndefinedValueRootIndex);
  __ b(eq, &no_info);

  __ ldr(r3, FieldMemOperand(r2, AllocationSite::kTransitionInfoOffset));
  __ SmiUntag(r3);
  STATIC_ASSERT(AllocationSite::ElementsKindBits::kShift == 0);
  __ and_(r3, r3, Operand(AllocationSite::ElementsKindBits::kMask));
  GenerateDispatchToArrayStub(masm, DONT_OVERRIDE);

  __ bind(&no_info);
  GenerateDispatchToArrayStub(masm, DISABLE_ALLOCATION_SITES);
}


void InternalArrayConstructorStub::GenerateCase(
    MacroAssembler* masm, ElementsKind kind) {
  __ cmp(r0, Operand(1));

  InternalArrayNoArgumentConstructorStub stub0(isolate(), kind);
  __ TailCallStub(&stub0, lo);

  InternalArrayNArgumentsConstructorStub stubN(isolate(), kind);
  __ TailCallStub(&stubN, hi);

  if (IsFastPackedElementsKind(kind)) {
    // We might need to create a holey array
    // look at the first argument
    __ ldr(r3, MemOperand(sp, 0));
    __ cmp(r3, Operand::Zero());

    InternalArraySingleArgumentConstructorStub
        stub1_holey(isolate(), GetHoleyElementsKind(kind));
    __ TailCallStub(&stub1_holey, ne);
  }

  InternalArraySingleArgumentConstructorStub stub1(isolate(), kind);
  __ TailCallStub(&stub1);
}


void InternalArrayConstructorStub::Generate(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- r0 : argc
  //  -- r1 : constructor
  //  -- sp[0] : return address
  //  -- sp[4] : last argument
  // -----------------------------------

  if (FLAG_debug_code) {
    // The array construct code is only set for the global and natives
    // builtin Array functions which always have maps.

    // Initial map for the builtin Array function should be a map.
    __ ldr(r3, FieldMemOperand(r1, JSFunction::kPrototypeOrInitialMapOffset));
    // Will both indicate a NULL and a Smi.
    __ tst(r3, Operand(kSmiTagMask));
    __ Assert(ne, kUnexpectedInitialMapForArrayFunction);
    __ CompareObjectType(r3, r3, r4, MAP_TYPE);
    __ Assert(eq, kUnexpectedInitialMapForArrayFunction);
  }

  // Figure out the right elements kind
  __ ldr(r3, FieldMemOperand(r1, JSFunction::kPrototypeOrInitialMapOffset));
  // Load the map's "bit field 2" into |result|. We only need the first byte,
  // but the following bit field extraction takes care of that anyway.
  __ ldr(r3, FieldMemOperand(r3, Map::kBitField2Offset));
  // Retrieve elements_kind from bit field 2.
  __ DecodeField<Map::ElementsKindBits>(r3);

  if (FLAG_debug_code) {
    Label done;
    __ cmp(r3, Operand(FAST_ELEMENTS));
    __ b(eq, &done);
    __ cmp(r3, Operand(FAST_HOLEY_ELEMENTS));
    __ Assert(eq,
              kInvalidElementsKindForInternalArrayOrInternalPackedArray);
    __ bind(&done);
  }

  Label fast_elements_case;
  __ cmp(r3, Operand(FAST_ELEMENTS));
  __ b(eq, &fast_elements_case);
  GenerateCase(masm, FAST_HOLEY_ELEMENTS);

  __ bind(&fast_elements_case);
  GenerateCase(masm, FAST_ELEMENTS);
}


void CallApiFunctionStub::Generate(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- r0                  : callee
  //  -- r4                  : call_data
  //  -- r2                  : holder
  //  -- r1                  : api_function_address
  //  -- cp                  : context
  //  --
  //  -- sp[0]               : last argument
  //  -- ...
  //  -- sp[(argc - 1)* 4]   : first argument
  //  -- sp[argc * 4]        : receiver
  // -----------------------------------

  Register callee = r0;
  Register call_data = r4;
  Register holder = r2;
  Register api_function_address = r1;
  Register context = cp;

  int argc = this->argc();
  bool is_store = this->is_store();
  bool call_data_undefined = this->call_data_undefined();

  typedef FunctionCallbackArguments FCA;

  STATIC_ASSERT(FCA::kContextSaveIndex == 6);
  STATIC_ASSERT(FCA::kCalleeIndex == 5);
  STATIC_ASSERT(FCA::kDataIndex == 4);
  STATIC_ASSERT(FCA::kReturnValueOffset == 3);
  STATIC_ASSERT(FCA::kReturnValueDefaultValueIndex == 2);
  STATIC_ASSERT(FCA::kIsolateIndex == 1);
  STATIC_ASSERT(FCA::kHolderIndex == 0);
  STATIC_ASSERT(FCA::kArgsLength == 7);

  // context save
  __ push(context);
  // load context from callee
  __ ldr(context, FieldMemOperand(callee, JSFunction::kContextOffset));

  // callee
  __ push(callee);

  // call data
  __ push(call_data);

  Register scratch = call_data;
  if (!call_data_undefined) {
    __ LoadRoot(scratch, Heap::kUndefinedValueRootIndex);
  }
  // return value
  __ push(scratch);
  // return value default
  __ push(scratch);
  // isolate
  __ mov(scratch,
         Operand(ExternalReference::isolate_address(isolate())));
  __ push(scratch);
  // holder
  __ push(holder);

  // Prepare arguments.
  __ mov(scratch, sp);

  // Allocate the v8::Arguments structure in the arguments' space since
  // it's not controlled by GC.
  const int kApiStackSpace = 4;

  FrameScope frame_scope(masm, StackFrame::MANUAL);
  __ EnterExitFrame(false, kApiStackSpace);

  DCHECK(!api_function_address.is(r0) && !scratch.is(r0));
  // r0 = FunctionCallbackInfo&
  // Arguments is after the return address.
  __ add(r0, sp, Operand(1 * kPointerSize));
  // FunctionCallbackInfo::implicit_args_
  __ str(scratch, MemOperand(r0, 0 * kPointerSize));
  // FunctionCallbackInfo::values_
  __ add(ip, scratch, Operand((FCA::kArgsLength - 1 + argc) * kPointerSize));
  __ str(ip, MemOperand(r0, 1 * kPointerSize));
  // FunctionCallbackInfo::length_ = argc
  __ mov(ip, Operand(argc));
  __ str(ip, MemOperand(r0, 2 * kPointerSize));
  // FunctionCallbackInfo::is_construct_call = 0
  __ mov(ip, Operand::Zero());
  __ str(ip, MemOperand(r0, 3 * kPointerSize));

  const int kStackUnwindSpace = argc + FCA::kArgsLength + 1;
  ExternalReference thunk_ref =
      ExternalReference::invoke_function_callback(isolate());

  AllowExternalCallThatCantCauseGC scope(masm);
  MemOperand context_restore_operand(
      fp, (2 + FCA::kContextSaveIndex) * kPointerSize);
  // Stores return the first js argument
  int return_value_offset = 0;
  if (is_store) {
    return_value_offset = 2 + FCA::kArgsLength;
  } else {
    return_value_offset = 2 + FCA::kReturnValueOffset;
  }
  MemOperand return_value_operand(fp, return_value_offset * kPointerSize);

  __ CallApiFunctionAndReturn(api_function_address,
                              thunk_ref,
                              kStackUnwindSpace,
                              return_value_operand,
                              &context_restore_operand);
}


void CallApiGetterStub::Generate(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- sp[0]                  : name
  //  -- sp[4 - kArgsLength*4]  : PropertyCallbackArguments object
  //  -- ...
  //  -- r2                     : api_function_address
  // -----------------------------------

  Register api_function_address = ApiGetterDescriptor::function_address();
  DCHECK(api_function_address.is(r2));

  __ mov(r0, sp);  // r0 = Handle<Name>
  __ add(r1, r0, Operand(1 * kPointerSize));  // r1 = PCA

  const int kApiStackSpace = 1;
  FrameScope frame_scope(masm, StackFrame::MANUAL);
  __ EnterExitFrame(false, kApiStackSpace);

  // Create PropertyAccessorInfo instance on the stack above the exit frame with
  // r1 (internal::Object** args_) as the data.
  __ str(r1, MemOperand(sp, 1 * kPointerSize));
  __ add(r1, sp, Operand(1 * kPointerSize));  // r1 = AccessorInfo&

  const int kStackUnwindSpace = PropertyCallbackArguments::kArgsLength + 1;

  ExternalReference thunk_ref =
      ExternalReference::invoke_accessor_getter_callback(isolate());
  __ CallApiFunctionAndReturn(api_function_address,
                              thunk_ref,
                              kStackUnwindSpace,
                              MemOperand(fp, 6 * kPointerSize),
                              NULL);
}


#undef __

} }  // namespace v8::internal

#endif  // V8_TARGET_ARCH_ARM
