// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/v8.h"

#if V8_TARGET_ARCH_MIPS64

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
    descriptor->Initialize(a0, deopt_handler, constant_stack_parameter_count,
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
    descriptor->Initialize(a0, deopt_handler, constant_stack_parameter_count,
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
                                          Condition cc);
static void EmitSmiNonsmiComparison(MacroAssembler* masm,
                                    Register lhs,
                                    Register rhs,
                                    Label* rhs_not_nan,
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
    FrameScope scope(masm, StackFrame::INTERNAL);
    DCHECK((param_count == 0) ||
           a0.is(descriptor.GetEnvironmentParameterRegister(param_count - 1)));
    // Push arguments, adjust sp.
    __ Dsubu(sp, sp, Operand(param_count * kPointerSize));
    for (int i = 0; i < param_count; ++i) {
      // Store argument to stack.
      __ sd(descriptor.GetEnvironmentParameterRegister(i),
            MemOperand(sp, (param_count - 1 - i) * kPointerSize));
    }
    __ CallExternalReference(miss, param_count);
  }

  __ Ret();
}


void DoubleToIStub::Generate(MacroAssembler* masm) {
  Label out_of_range, only_low, negate, done;
  Register input_reg = source();
  Register result_reg = destination();

  int double_offset = offset();
  // Account for saved regs if input is sp.
  if (input_reg.is(sp)) double_offset += 3 * kPointerSize;

  Register scratch =
      GetRegisterThatIsNotOneOf(input_reg, result_reg);
  Register scratch2 =
      GetRegisterThatIsNotOneOf(input_reg, result_reg, scratch);
  Register scratch3 =
      GetRegisterThatIsNotOneOf(input_reg, result_reg, scratch, scratch2);
  DoubleRegister double_scratch = kLithiumScratchDouble;

  __ Push(scratch, scratch2, scratch3);
  if (!skip_fastpath()) {
    // Load double input.
    __ ldc1(double_scratch, MemOperand(input_reg, double_offset));

    // Clear cumulative exception flags and save the FCSR.
    __ cfc1(scratch2, FCSR);
    __ ctc1(zero_reg, FCSR);

    // Try a conversion to a signed integer.
    __ Trunc_w_d(double_scratch, double_scratch);
    // Move the converted value into the result register.
    __ mfc1(scratch3, double_scratch);

    // Retrieve and restore the FCSR.
    __ cfc1(scratch, FCSR);
    __ ctc1(scratch2, FCSR);

    // Check for overflow and NaNs.
    __ And(
        scratch, scratch,
        kFCSROverflowFlagMask | kFCSRUnderflowFlagMask
           | kFCSRInvalidOpFlagMask);
    // If we had no exceptions then set result_reg and we are done.
    Label error;
    __ Branch(&error, ne, scratch, Operand(zero_reg));
    __ Move(result_reg, scratch3);
    __ Branch(&done);
    __ bind(&error);
  }

  // Load the double value and perform a manual truncation.
  Register input_high = scratch2;
  Register input_low = scratch3;

  __ lw(input_low, MemOperand(input_reg, double_offset));
  __ lw(input_high, MemOperand(input_reg, double_offset + kIntSize));

  Label normal_exponent, restore_sign;
  // Extract the biased exponent in result.
  __ Ext(result_reg,
         input_high,
         HeapNumber::kExponentShift,
         HeapNumber::kExponentBits);

  // Check for Infinity and NaNs, which should return 0.
  __ Subu(scratch, result_reg, HeapNumber::kExponentMask);
  __ Movz(result_reg, zero_reg, scratch);
  __ Branch(&done, eq, scratch, Operand(zero_reg));

  // Express exponent as delta to (number of mantissa bits + 31).
  __ Subu(result_reg,
          result_reg,
          Operand(HeapNumber::kExponentBias + HeapNumber::kMantissaBits + 31));

  // If the delta is strictly positive, all bits would be shifted away,
  // which means that we can return 0.
  __ Branch(&normal_exponent, le, result_reg, Operand(zero_reg));
  __ mov(result_reg, zero_reg);
  __ Branch(&done);

  __ bind(&normal_exponent);
  const int kShiftBase = HeapNumber::kNonMantissaBitsInTopWord - 1;
  // Calculate shift.
  __ Addu(scratch, result_reg, Operand(kShiftBase + HeapNumber::kMantissaBits));

  // Save the sign.
  Register sign = result_reg;
  result_reg = no_reg;
  __ And(sign, input_high, Operand(HeapNumber::kSignMask));

  // On ARM shifts > 31 bits are valid and will result in zero. On MIPS we need
  // to check for this specific case.
  Label high_shift_needed, high_shift_done;
  __ Branch(&high_shift_needed, lt, scratch, Operand(32));
  __ mov(input_high, zero_reg);
  __ Branch(&high_shift_done);
  __ bind(&high_shift_needed);

  // Set the implicit 1 before the mantissa part in input_high.
  __ Or(input_high,
        input_high,
        Operand(1 << HeapNumber::kMantissaBitsInTopWord));
  // Shift the mantissa bits to the correct position.
  // We don't need to clear non-mantissa bits as they will be shifted away.
  // If they weren't, it would mean that the answer is in the 32bit range.
  __ sllv(input_high, input_high, scratch);

  __ bind(&high_shift_done);

  // Replace the shifted bits with bits from the lower mantissa word.
  Label pos_shift, shift_done;
  __ li(at, 32);
  __ subu(scratch, at, scratch);
  __ Branch(&pos_shift, ge, scratch, Operand(zero_reg));

  // Negate scratch.
  __ Subu(scratch, zero_reg, scratch);
  __ sllv(input_low, input_low, scratch);
  __ Branch(&shift_done);

  __ bind(&pos_shift);
  __ srlv(input_low, input_low, scratch);

  __ bind(&shift_done);
  __ Or(input_high, input_high, Operand(input_low));
  // Restore sign if necessary.
  __ mov(scratch, sign);
  result_reg = sign;
  sign = no_reg;
  __ Subu(result_reg, zero_reg, input_high);
  __ Movz(result_reg, input_high, scratch);

  __ bind(&done);

  __ Pop(scratch, scratch2, scratch3);
  __ Ret();
}


void WriteInt32ToHeapNumberStub::GenerateFixedRegStubsAheadOfTime(
    Isolate* isolate) {
  WriteInt32ToHeapNumberStub stub1(isolate, a1, v0, a2, a3);
  WriteInt32ToHeapNumberStub stub2(isolate, a2, v0, a3, a0);
  stub1.GetCode();
  stub2.GetCode();
}


// See comment for class, this does NOT work for int32's that are in Smi range.
void WriteInt32ToHeapNumberStub::Generate(MacroAssembler* masm) {
  Label max_negative_int;
  // the_int_ has the answer which is a signed int32 but not a Smi.
  // We test for the special value that has a different exponent.
  STATIC_ASSERT(HeapNumber::kSignMask == 0x80000000u);
  // Test sign, and save for later conditionals.
  __ And(sign(), the_int(), Operand(0x80000000u));
  __ Branch(&max_negative_int, eq, the_int(), Operand(0x80000000u));

  // Set up the correct exponent in scratch_.  All non-Smi int32s have the same.
  // A non-Smi integer is 1.xxx * 2^30 so the exponent is 30 (biased).
  uint32_t non_smi_exponent =
      (HeapNumber::kExponentBias + 30) << HeapNumber::kExponentShift;
  __ li(scratch(), Operand(non_smi_exponent));
  // Set the sign bit in scratch_ if the value was negative.
  __ or_(scratch(), scratch(), sign());
  // Subtract from 0 if the value was negative.
  __ subu(at, zero_reg, the_int());
  __ Movn(the_int(), at, sign());
  // We should be masking the implict first digit of the mantissa away here,
  // but it just ends up combining harmlessly with the last digit of the
  // exponent that happens to be 1.  The sign bit is 0 so we shift 10 to get
  // the most significant 1 to hit the last bit of the 12 bit sign and exponent.
  DCHECK(((1 << HeapNumber::kExponentShift) & non_smi_exponent) != 0);
  const int shift_distance = HeapNumber::kNonMantissaBitsInTopWord - 2;
  __ srl(at, the_int(), shift_distance);
  __ or_(scratch(), scratch(), at);
  __ sw(scratch(), FieldMemOperand(the_heap_number(),
                                   HeapNumber::kExponentOffset));
  __ sll(scratch(), the_int(), 32 - shift_distance);
  __ Ret(USE_DELAY_SLOT);
  __ sw(scratch(), FieldMemOperand(the_heap_number(),
                                   HeapNumber::kMantissaOffset));

  __ bind(&max_negative_int);
  // The max negative int32 is stored as a positive number in the mantissa of
  // a double because it uses a sign bit instead of using two's complement.
  // The actual mantissa bits stored are all 0 because the implicit most
  // significant 1 bit is not stored.
  non_smi_exponent += 1 << HeapNumber::kExponentShift;
  __ li(scratch(), Operand(HeapNumber::kSignMask | non_smi_exponent));
  __ sw(scratch(),
        FieldMemOperand(the_heap_number(), HeapNumber::kExponentOffset));
  __ mov(scratch(), zero_reg);
  __ Ret(USE_DELAY_SLOT);
  __ sw(scratch(),
        FieldMemOperand(the_heap_number(), HeapNumber::kMantissaOffset));
}


// Handle the case where the lhs and rhs are the same object.
// Equality is almost reflexive (everything but NaN), so this is a test
// for "identity and not NaN".
static void EmitIdenticalObjectComparison(MacroAssembler* masm,
                                          Label* slow,
                                          Condition cc) {
  Label not_identical;
  Label heap_number, return_equal;
  Register exp_mask_reg = t1;

  __ Branch(&not_identical, ne, a0, Operand(a1));

  __ li(exp_mask_reg, Operand(HeapNumber::kExponentMask));

  // Test for NaN. Sadly, we can't just compare to Factory::nan_value(),
  // so we do the second best thing - test it ourselves.
  // They are both equal and they are not both Smis so both of them are not
  // Smis. If it's not a heap number, then return equal.
  if (cc == less || cc == greater) {
    __ GetObjectType(a0, t0, t0);
    __ Branch(slow, greater, t0, Operand(FIRST_SPEC_OBJECT_TYPE));
  } else {
    __ GetObjectType(a0, t0, t0);
    __ Branch(&heap_number, eq, t0, Operand(HEAP_NUMBER_TYPE));
    // Comparing JS objects with <=, >= is complicated.
    if (cc != eq) {
    __ Branch(slow, greater, t0, Operand(FIRST_SPEC_OBJECT_TYPE));
      // Normally here we fall through to return_equal, but undefined is
      // special: (undefined == undefined) == true, but
      // (undefined <= undefined) == false!  See ECMAScript 11.8.5.
      if (cc == less_equal || cc == greater_equal) {
        __ Branch(&return_equal, ne, t0, Operand(ODDBALL_TYPE));
        __ LoadRoot(a6, Heap::kUndefinedValueRootIndex);
        __ Branch(&return_equal, ne, a0, Operand(a6));
        DCHECK(is_int16(GREATER) && is_int16(LESS));
        __ Ret(USE_DELAY_SLOT);
        if (cc == le) {
          // undefined <= undefined should fail.
          __ li(v0, Operand(GREATER));
        } else  {
          // undefined >= undefined should fail.
          __ li(v0, Operand(LESS));
        }
      }
    }
  }

  __ bind(&return_equal);
  DCHECK(is_int16(GREATER) && is_int16(LESS));
  __ Ret(USE_DELAY_SLOT);
  if (cc == less) {
    __ li(v0, Operand(GREATER));  // Things aren't less than themselves.
  } else if (cc == greater) {
    __ li(v0, Operand(LESS));     // Things aren't greater than themselves.
  } else {
    __ mov(v0, zero_reg);         // Things are <=, >=, ==, === themselves.
  }
  // For less and greater we don't have to check for NaN since the result of
  // x < x is false regardless.  For the others here is some code to check
  // for NaN.
  if (cc != lt && cc != gt) {
    __ bind(&heap_number);
    // It is a heap number, so return non-equal if it's NaN and equal if it's
    // not NaN.

    // The representation of NaN values has all exponent bits (52..62) set,
    // and not all mantissa bits (0..51) clear.
    // Read top bits of double representation (second word of value).
    __ lwu(a6, FieldMemOperand(a0, HeapNumber::kExponentOffset));
    // Test that exponent bits are all set.
    __ And(a7, a6, Operand(exp_mask_reg));
    // If all bits not set (ne cond), then not a NaN, objects are equal.
    __ Branch(&return_equal, ne, a7, Operand(exp_mask_reg));

    // Shift out flag and all exponent bits, retaining only mantissa.
    __ sll(a6, a6, HeapNumber::kNonMantissaBitsInTopWord);
    // Or with all low-bits of mantissa.
    __ lwu(a7, FieldMemOperand(a0, HeapNumber::kMantissaOffset));
    __ Or(v0, a7, Operand(a6));
    // For equal we already have the right value in v0:  Return zero (equal)
    // if all bits in mantissa are zero (it's an Infinity) and non-zero if
    // not (it's a NaN).  For <= and >= we need to load v0 with the failing
    // value if it's a NaN.
    if (cc != eq) {
      // All-zero means Infinity means equal.
      __ Ret(eq, v0, Operand(zero_reg));
      DCHECK(is_int16(GREATER) && is_int16(LESS));
      __ Ret(USE_DELAY_SLOT);
      if (cc == le) {
        __ li(v0, Operand(GREATER));  // NaN <= NaN should fail.
      } else {
        __ li(v0, Operand(LESS));     // NaN >= NaN should fail.
      }
    }
  }
  // No fall through here.

  __ bind(&not_identical);
}


static void EmitSmiNonsmiComparison(MacroAssembler* masm,
                                    Register lhs,
                                    Register rhs,
                                    Label* both_loaded_as_doubles,
                                    Label* slow,
                                    bool strict) {
  DCHECK((lhs.is(a0) && rhs.is(a1)) ||
         (lhs.is(a1) && rhs.is(a0)));

  Label lhs_is_smi;
  __ JumpIfSmi(lhs, &lhs_is_smi);
  // Rhs is a Smi.
  // Check whether the non-smi is a heap number.
  __ GetObjectType(lhs, t0, t0);
  if (strict) {
    // If lhs was not a number and rhs was a Smi then strict equality cannot
    // succeed. Return non-equal (lhs is already not zero).
    __ Ret(USE_DELAY_SLOT, ne, t0, Operand(HEAP_NUMBER_TYPE));
    __ mov(v0, lhs);
  } else {
    // Smi compared non-strictly with a non-Smi non-heap-number. Call
    // the runtime.
    __ Branch(slow, ne, t0, Operand(HEAP_NUMBER_TYPE));
  }
  // Rhs is a smi, lhs is a number.
  // Convert smi rhs to double.
  __ SmiUntag(at, rhs);
  __ mtc1(at, f14);
  __ cvt_d_w(f14, f14);
  __ ldc1(f12, FieldMemOperand(lhs, HeapNumber::kValueOffset));

  // We now have both loaded as doubles.
  __ jmp(both_loaded_as_doubles);

  __ bind(&lhs_is_smi);
  // Lhs is a Smi.  Check whether the non-smi is a heap number.
  __ GetObjectType(rhs, t0, t0);
  if (strict) {
    // If lhs was not a number and rhs was a Smi then strict equality cannot
    // succeed. Return non-equal.
    __ Ret(USE_DELAY_SLOT, ne, t0, Operand(HEAP_NUMBER_TYPE));
    __ li(v0, Operand(1));
  } else {
    // Smi compared non-strictly with a non-Smi non-heap-number. Call
    // the runtime.
    __ Branch(slow, ne, t0, Operand(HEAP_NUMBER_TYPE));
  }

  // Lhs is a smi, rhs is a number.
  // Convert smi lhs to double.
  __ SmiUntag(at, lhs);
  __ mtc1(at, f12);
  __ cvt_d_w(f12, f12);
  __ ldc1(f14, FieldMemOperand(rhs, HeapNumber::kValueOffset));
  // Fall through to both_loaded_as_doubles.
}


static void EmitStrictTwoHeapObjectCompare(MacroAssembler* masm,
                                           Register lhs,
                                           Register rhs) {
    // If either operand is a JS object or an oddball value, then they are
    // not equal since their pointers are different.
    // There is no test for undetectability in strict equality.
    STATIC_ASSERT(LAST_TYPE == LAST_SPEC_OBJECT_TYPE);
    Label first_non_object;
    // Get the type of the first operand into a2 and compare it with
    // FIRST_SPEC_OBJECT_TYPE.
    __ GetObjectType(lhs, a2, a2);
    __ Branch(&first_non_object, less, a2, Operand(FIRST_SPEC_OBJECT_TYPE));

    // Return non-zero.
    Label return_not_equal;
    __ bind(&return_not_equal);
    __ Ret(USE_DELAY_SLOT);
    __ li(v0, Operand(1));

    __ bind(&first_non_object);
    // Check for oddballs: true, false, null, undefined.
    __ Branch(&return_not_equal, eq, a2, Operand(ODDBALL_TYPE));

    __ GetObjectType(rhs, a3, a3);
    __ Branch(&return_not_equal, greater, a3, Operand(FIRST_SPEC_OBJECT_TYPE));

    // Check for oddballs: true, false, null, undefined.
    __ Branch(&return_not_equal, eq, a3, Operand(ODDBALL_TYPE));

    // Now that we have the types we might as well check for
    // internalized-internalized.
    STATIC_ASSERT(kInternalizedTag == 0 && kStringTag == 0);
    __ Or(a2, a2, Operand(a3));
    __ And(at, a2, Operand(kIsNotStringMask | kIsNotInternalizedMask));
    __ Branch(&return_not_equal, eq, at, Operand(zero_reg));
}


static void EmitCheckForTwoHeapNumbers(MacroAssembler* masm,
                                       Register lhs,
                                       Register rhs,
                                       Label* both_loaded_as_doubles,
                                       Label* not_heap_numbers,
                                       Label* slow) {
  __ GetObjectType(lhs, a3, a2);
  __ Branch(not_heap_numbers, ne, a2, Operand(HEAP_NUMBER_TYPE));
  __ ld(a2, FieldMemOperand(rhs, HeapObject::kMapOffset));
  // If first was a heap number & second wasn't, go to slow case.
  __ Branch(slow, ne, a3, Operand(a2));

  // Both are heap numbers. Load them up then jump to the code we have
  // for that.
  __ ldc1(f12, FieldMemOperand(lhs, HeapNumber::kValueOffset));
  __ ldc1(f14, FieldMemOperand(rhs, HeapNumber::kValueOffset));

  __ jmp(both_loaded_as_doubles);
}


// Fast negative check for internalized-to-internalized equality.
static void EmitCheckForInternalizedStringsOrObjects(MacroAssembler* masm,
                                                     Register lhs,
                                                     Register rhs,
                                                     Label* possible_strings,
                                                     Label* not_both_strings) {
  DCHECK((lhs.is(a0) && rhs.is(a1)) ||
         (lhs.is(a1) && rhs.is(a0)));

  // a2 is object type of rhs.
  Label object_test;
  STATIC_ASSERT(kInternalizedTag == 0 && kStringTag == 0);
  __ And(at, a2, Operand(kIsNotStringMask));
  __ Branch(&object_test, ne, at, Operand(zero_reg));
  __ And(at, a2, Operand(kIsNotInternalizedMask));
  __ Branch(possible_strings, ne, at, Operand(zero_reg));
  __ GetObjectType(rhs, a3, a3);
  __ Branch(not_both_strings, ge, a3, Operand(FIRST_NONSTRING_TYPE));
  __ And(at, a3, Operand(kIsNotInternalizedMask));
  __ Branch(possible_strings, ne, at, Operand(zero_reg));

  // Both are internalized strings. We already checked they weren't the same
  // pointer so they are not equal.
  __ Ret(USE_DELAY_SLOT);
  __ li(v0, Operand(1));   // Non-zero indicates not equal.

  __ bind(&object_test);
  __ Branch(not_both_strings, lt, a2, Operand(FIRST_SPEC_OBJECT_TYPE));
  __ GetObjectType(rhs, a2, a3);
  __ Branch(not_both_strings, lt, a3, Operand(FIRST_SPEC_OBJECT_TYPE));

  // If both objects are undetectable, they are equal.  Otherwise, they
  // are not equal, since they are different objects and an object is not
  // equal to undefined.
  __ ld(a3, FieldMemOperand(lhs, HeapObject::kMapOffset));
  __ lbu(a2, FieldMemOperand(a2, Map::kBitFieldOffset));
  __ lbu(a3, FieldMemOperand(a3, Map::kBitFieldOffset));
  __ and_(a0, a2, a3);
  __ And(a0, a0, Operand(1 << Map::kIsUndetectable));
  __ Ret(USE_DELAY_SLOT);
  __ xori(v0, a0, 1 << Map::kIsUndetectable);
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
  // We could be strict about internalized/string here, but as long as
  // hydrogen doesn't care, the stub doesn't have to care either.
  __ bind(&ok);
}


// On entry a1 and a2 are the values to be compared.
// On exit a0 is 0, positive or negative to indicate the result of
// the comparison.
void CompareICStub::GenerateGeneric(MacroAssembler* masm) {
  Register lhs = a1;
  Register rhs = a0;
  Condition cc = GetCondition();

  Label miss;
  CompareICStub_CheckInputType(masm, lhs, a2, left(), &miss);
  CompareICStub_CheckInputType(masm, rhs, a3, right(), &miss);

  Label slow;  // Call builtin.
  Label not_smis, both_loaded_as_doubles;

  Label not_two_smis, smi_done;
  __ Or(a2, a1, a0);
  __ JumpIfNotSmi(a2, &not_two_smis);
  __ SmiUntag(a1);
  __ SmiUntag(a0);

  __ Ret(USE_DELAY_SLOT);
  __ dsubu(v0, a1, a0);
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
  __ And(a6, lhs, Operand(rhs));
  __ JumpIfNotSmi(a6, &not_smis, a4);
  // One operand is a smi. EmitSmiNonsmiComparison generates code that can:
  // 1) Return the answer.
  // 2) Go to slow.
  // 3) Fall through to both_loaded_as_doubles.
  // 4) Jump to rhs_not_nan.
  // In cases 3 and 4 we have found out we were dealing with a number-number
  // comparison and the numbers have been loaded into f12 and f14 as doubles,
  // or in GP registers (a0, a1, a2, a3) depending on the presence of the FPU.
  EmitSmiNonsmiComparison(masm, lhs, rhs,
                          &both_loaded_as_doubles, &slow, strict());

  __ bind(&both_loaded_as_doubles);
  // f12, f14 are the double representations of the left hand side
  // and the right hand side if we have FPU. Otherwise a2, a3 represent
  // left hand side and a0, a1 represent right hand side.

  Label nan;
  __ li(a4, Operand(LESS));
  __ li(a5, Operand(GREATER));
  __ li(a6, Operand(EQUAL));

  // Check if either rhs or lhs is NaN.
  __ BranchF(NULL, &nan, eq, f12, f14);

  // Check if LESS condition is satisfied. If true, move conditionally
  // result to v0.
  if (kArchVariant != kMips64r6) {
    __ c(OLT, D, f12, f14);
    __ Movt(v0, a4);
    // Use previous check to store conditionally to v0 oposite condition
    // (GREATER). If rhs is equal to lhs, this will be corrected in next
    // check.
    __ Movf(v0, a5);
    // Check if EQUAL condition is satisfied. If true, move conditionally
    // result to v0.
    __ c(EQ, D, f12, f14);
    __ Movt(v0, a6);
  } else {
    Label skip;
    __ BranchF(USE_DELAY_SLOT, &skip, NULL, lt, f12, f14);
    __ mov(v0, a4);  // Return LESS as result.

    __ BranchF(USE_DELAY_SLOT, &skip, NULL, eq, f12, f14);
    __ mov(v0, a6);  // Return EQUAL as result.

    __ mov(v0, a5);  // Return GREATER as result.
    __ bind(&skip);
  }
  __ Ret();

  __ bind(&nan);
  // NaN comparisons always fail.
  // Load whatever we need in v0 to make the comparison fail.
  DCHECK(is_int16(GREATER) && is_int16(LESS));
  __ Ret(USE_DELAY_SLOT);
  if (cc == lt || cc == le) {
    __ li(v0, Operand(GREATER));
  } else {
    __ li(v0, Operand(LESS));
  }


  __ bind(&not_smis);
  // At this point we know we are dealing with two different objects,
  // and neither of them is a Smi. The objects are in lhs_ and rhs_.
  if (strict()) {
    // This returns non-equal for some object types, or falls through if it
    // was not lucky.
    EmitStrictTwoHeapObjectCompare(masm, lhs, rhs);
  }

  Label check_for_internalized_strings;
  Label flat_string_check;
  // Check for heap-number-heap-number comparison. Can jump to slow case,
  // or load both doubles and jump to the code that handles
  // that case. If the inputs are not doubles then jumps to
  // check_for_internalized_strings.
  // In this case a2 will contain the type of lhs_.
  EmitCheckForTwoHeapNumbers(masm,
                             lhs,
                             rhs,
                             &both_loaded_as_doubles,
                             &check_for_internalized_strings,
                             &flat_string_check);

  __ bind(&check_for_internalized_strings);
  if (cc == eq && !strict()) {
    // Returns an answer for two internalized strings or two
    // detectable objects.
    // Otherwise jumps to string case or not both strings case.
    // Assumes that a2 is the type of lhs_ on entry.
    EmitCheckForInternalizedStringsOrObjects(
        masm, lhs, rhs, &flat_string_check, &slow);
  }

  // Check for both being sequential one-byte strings,
  // and inline if that is the case.
  __ bind(&flat_string_check);

  __ JumpIfNonSmisNotBothSequentialOneByteStrings(lhs, rhs, a2, a3, &slow);

  __ IncrementCounter(isolate()->counters()->string_compare_native(), 1, a2,
                      a3);
  if (cc == eq) {
    StringHelper::GenerateFlatOneByteStringEquals(masm, lhs, rhs, a2, a3, a4);
  } else {
    StringHelper::GenerateCompareFlatOneByteStrings(masm, lhs, rhs, a2, a3, a4,
                                                    a5);
  }
  // Never falls through to here.

  __ bind(&slow);
  // Prepare for call to builtin. Push object pointers, a0 (lhs) first,
  // a1 (rhs) second.
  __ Push(lhs, rhs);
  // Figure out which native to call and setup the arguments.
  Builtins::JavaScript native;
  if (cc == eq) {
    native = strict() ? Builtins::STRICT_EQUALS : Builtins::EQUALS;
  } else {
    native = Builtins::COMPARE;
    int ncr;  // NaN compare result.
    if (cc == lt || cc == le) {
      ncr = GREATER;
    } else {
      DCHECK(cc == gt || cc == ge);  // Remaining cases.
      ncr = LESS;
    }
    __ li(a0, Operand(Smi::FromInt(ncr)));
    __ push(a0);
  }

  // Call the native; it returns -1 (less), 0 (equal), or 1 (greater)
  // tagged as a small integer.
  __ InvokeBuiltin(native, JUMP_FUNCTION);

  __ bind(&miss);
  GenerateMiss(masm);
}


void StoreRegistersStateStub::Generate(MacroAssembler* masm) {
  __ mov(t9, ra);
  __ pop(ra);
  __ PushSafepointRegisters();
  __ Jump(t9);
}


void RestoreRegistersStateStub::Generate(MacroAssembler* masm) {
  __ mov(t9, ra);
  __ pop(ra);
  __ PopSafepointRegisters();
  __ Jump(t9);
}


void StoreBufferOverflowStub::Generate(MacroAssembler* masm) {
  // We don't allow a GC during a store buffer overflow so there is no need to
  // store the registers in any particular way, but we do have to store and
  // restore them.
  __ MultiPush(kJSCallerSaved | ra.bit());
  if (save_doubles()) {
    __ MultiPushFPU(kCallerSavedFPU);
  }
  const int argument_count = 1;
  const int fp_argument_count = 0;
  const Register scratch = a1;

  AllowExternalCallThatCantCauseGC scope(masm);
  __ PrepareCallCFunction(argument_count, fp_argument_count, scratch);
  __ li(a0, Operand(ExternalReference::isolate_address(isolate())));
  __ CallCFunction(
      ExternalReference::store_buffer_overflow_function(isolate()),
      argument_count);
  if (save_doubles()) {
    __ MultiPopFPU(kCallerSavedFPU);
  }

  __ MultiPop(kJSCallerSaved | ra.bit());
  __ Ret();
}


void MathPowStub::Generate(MacroAssembler* masm) {
  const Register base = a1;
  const Register exponent = MathPowTaggedDescriptor::exponent();
  DCHECK(exponent.is(a2));
  const Register heapnumbermap = a5;
  const Register heapnumber = v0;
  const DoubleRegister double_base = f2;
  const DoubleRegister double_exponent = f4;
  const DoubleRegister double_result = f0;
  const DoubleRegister double_scratch = f6;
  const FPURegister single_scratch = f8;
  const Register scratch = t1;
  const Register scratch2 = a7;

  Label call_runtime, done, int_exponent;
  if (exponent_type() == ON_STACK) {
    Label base_is_smi, unpack_exponent;
    // The exponent and base are supplied as arguments on the stack.
    // This can only happen if the stub is called from non-optimized code.
    // Load input parameters from stack to double registers.
    __ ld(base, MemOperand(sp, 1 * kPointerSize));
    __ ld(exponent, MemOperand(sp, 0 * kPointerSize));

    __ LoadRoot(heapnumbermap, Heap::kHeapNumberMapRootIndex);

    __ UntagAndJumpIfSmi(scratch, base, &base_is_smi);
    __ ld(scratch, FieldMemOperand(base, JSObject::kMapOffset));
    __ Branch(&call_runtime, ne, scratch, Operand(heapnumbermap));

    __ ldc1(double_base, FieldMemOperand(base, HeapNumber::kValueOffset));
    __ jmp(&unpack_exponent);

    __ bind(&base_is_smi);
    __ mtc1(scratch, single_scratch);
    __ cvt_d_w(double_base, single_scratch);
    __ bind(&unpack_exponent);

    __ UntagAndJumpIfSmi(scratch, exponent, &int_exponent);

    __ ld(scratch, FieldMemOperand(exponent, JSObject::kMapOffset));
    __ Branch(&call_runtime, ne, scratch, Operand(heapnumbermap));
    __ ldc1(double_exponent,
            FieldMemOperand(exponent, HeapNumber::kValueOffset));
  } else if (exponent_type() == TAGGED) {
    // Base is already in double_base.
    __ UntagAndJumpIfSmi(scratch, exponent, &int_exponent);

    __ ldc1(double_exponent,
            FieldMemOperand(exponent, HeapNumber::kValueOffset));
  }

  if (exponent_type() != INTEGER) {
    Label int_exponent_convert;
    // Detect integer exponents stored as double.
    __ EmitFPUTruncate(kRoundToMinusInf,
                       scratch,
                       double_exponent,
                       at,
                       double_scratch,
                       scratch2,
                       kCheckForInexactConversion);
    // scratch2 == 0 means there was no conversion error.
    __ Branch(&int_exponent_convert, eq, scratch2, Operand(zero_reg));

    if (exponent_type() == ON_STACK) {
      // Detect square root case.  Crankshaft detects constant +/-0.5 at
      // compile time and uses DoMathPowHalf instead.  We then skip this check
      // for non-constant cases of +/-0.5 as these hardly occur.
      Label not_plus_half;

      // Test for 0.5.
      __ Move(double_scratch, 0.5);
      __ BranchF(USE_DELAY_SLOT,
                 &not_plus_half,
                 NULL,
                 ne,
                 double_exponent,
                 double_scratch);
      // double_scratch can be overwritten in the delay slot.
      // Calculates square root of base.  Check for the special case of
      // Math.pow(-Infinity, 0.5) == Infinity (ECMA spec, 15.8.2.13).
      __ Move(double_scratch, -V8_INFINITY);
      __ BranchF(USE_DELAY_SLOT, &done, NULL, eq, double_base, double_scratch);
      __ neg_d(double_result, double_scratch);

      // Add +0 to convert -0 to +0.
      __ add_d(double_scratch, double_base, kDoubleRegZero);
      __ sqrt_d(double_result, double_scratch);
      __ jmp(&done);

      __ bind(&not_plus_half);
      __ Move(double_scratch, -0.5);
      __ BranchF(USE_DELAY_SLOT,
                 &call_runtime,
                 NULL,
                 ne,
                 double_exponent,
                 double_scratch);
      // double_scratch can be overwritten in the delay slot.
      // Calculates square root of base.  Check for the special case of
      // Math.pow(-Infinity, -0.5) == 0 (ECMA spec, 15.8.2.13).
      __ Move(double_scratch, -V8_INFINITY);
      __ BranchF(USE_DELAY_SLOT, &done, NULL, eq, double_base, double_scratch);
      __ Move(double_result, kDoubleRegZero);

      // Add +0 to convert -0 to +0.
      __ add_d(double_scratch, double_base, kDoubleRegZero);
      __ Move(double_result, 1);
      __ sqrt_d(double_scratch, double_scratch);
      __ div_d(double_result, double_result, double_scratch);
      __ jmp(&done);
    }

    __ push(ra);
    {
      AllowExternalCallThatCantCauseGC scope(masm);
      __ PrepareCallCFunction(0, 2, scratch2);
      __ MovToFloatParameters(double_base, double_exponent);
      __ CallCFunction(
          ExternalReference::power_double_double_function(isolate()),
          0, 2);
    }
    __ pop(ra);
    __ MovFromFloatResult(double_result);
    __ jmp(&done);

    __ bind(&int_exponent_convert);
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

  __ mov_d(double_scratch, double_base);  // Back up base.
  __ Move(double_result, 1.0);

  // Get absolute value of exponent.
  Label positive_exponent;
  __ Branch(&positive_exponent, ge, scratch, Operand(zero_reg));
  __ Dsubu(scratch, zero_reg, scratch);
  __ bind(&positive_exponent);

  Label while_true, no_carry, loop_end;
  __ bind(&while_true);

  __ And(scratch2, scratch, 1);

  __ Branch(&no_carry, eq, scratch2, Operand(zero_reg));
  __ mul_d(double_result, double_result, double_scratch);
  __ bind(&no_carry);

  __ dsra(scratch, scratch, 1);

  __ Branch(&loop_end, eq, scratch, Operand(zero_reg));
  __ mul_d(double_scratch, double_scratch, double_scratch);

  __ Branch(&while_true);

  __ bind(&loop_end);

  __ Branch(&done, ge, exponent, Operand(zero_reg));
  __ Move(double_scratch, 1.0);
  __ div_d(double_result, double_scratch, double_result);
  // Test whether result is zero.  Bail out to check for subnormal result.
  // Due to subnormals, x^-y == (1/x)^y does not hold in all cases.
  __ BranchF(&done, NULL, ne, double_result, kDoubleRegZero);

  // double_exponent may not contain the exponent value if the input was a
  // smi.  We set it with exponent value before bailing out.
  __ mtc1(exponent, single_scratch);
  __ cvt_d_w(double_exponent, single_scratch);

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
    __ sdc1(double_result,
            FieldMemOperand(heapnumber, HeapNumber::kValueOffset));
    DCHECK(heapnumber.is(v0));
    __ IncrementCounter(counters->math_pow(), 1, scratch, scratch2);
    __ DropAndRet(2);
  } else {
    __ push(ra);
    {
      AllowExternalCallThatCantCauseGC scope(masm);
      __ PrepareCallCFunction(0, 2, scratch);
      __ MovToFloatParameters(double_base, double_exponent);
      __ CallCFunction(
          ExternalReference::power_double_double_function(isolate()),
          0, 2);
    }
    __ pop(ra);
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
  StoreRegistersStateStub::GenerateAheadOfTime(isolate);
  RestoreRegistersStateStub::GenerateAheadOfTime(isolate);
  BinaryOpICWithAllocationSiteStub::GenerateAheadOfTime(isolate);
}


void StoreRegistersStateStub::GenerateAheadOfTime(Isolate* isolate) {
  StoreRegistersStateStub stub(isolate);
  stub.GetCode();
}


void RestoreRegistersStateStub::GenerateAheadOfTime(Isolate* isolate) {
  RestoreRegistersStateStub stub(isolate);
  stub.GetCode();
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
  // Called from JavaScript; parameters are on stack as if calling JS function
  // s0: number of arguments including receiver
  // s1: size of arguments excluding receiver
  // s2: pointer to builtin function
  // fp: frame pointer    (restored after C call)
  // sp: stack pointer    (restored as callee's sp after C call)
  // cp: current context  (C callee-saved)

  ProfileEntryHookStub::MaybeCallEntryHook(masm);

  // NOTE: s0-s2 hold the arguments of this function instead of a0-a2.
  // The reason for this is that these arguments would need to be saved anyway
  // so it's faster to set them up directly.
  // See MacroAssembler::PrepareCEntryArgs and PrepareCEntryFunction.

  // Compute the argv pointer in a callee-saved register.
  __ Daddu(s1, sp, s1);

  // Enter the exit frame that transitions from JavaScript to C++.
  FrameScope scope(masm, StackFrame::MANUAL);
  __ EnterExitFrame(save_doubles());

  // s0: number of arguments  including receiver (C callee-saved)
  // s1: pointer to first argument (C callee-saved)
  // s2: pointer to builtin function (C callee-saved)

  // Prepare arguments for C routine.
  // a0 = argc
  __ mov(a0, s0);
  // a1 = argv (set in the delay slot after find_ra below).

  // We are calling compiled C/C++ code. a0 and a1 hold our two arguments. We
  // also need to reserve the 4 argument slots on the stack.

  __ AssertStackIsAligned();

  __ li(a2, Operand(ExternalReference::isolate_address(isolate())));

  // To let the GC traverse the return address of the exit frames, we need to
  // know where the return address is. The CEntryStub is unmovable, so
  // we can store the address on the stack to be able to find it again and
  // we never have to restore it, because it will not change.
  { Assembler::BlockTrampolinePoolScope block_trampoline_pool(masm);
    // This branch-and-link sequence is needed to find the current PC on mips,
    // saved to the ra register.
    // Use masm-> here instead of the double-underscore macro since extra
    // coverage code can interfere with the proper calculation of ra.
    Label find_ra;
    masm->bal(&find_ra);  // bal exposes branch delay slot.
    masm->mov(a1, s1);
    masm->bind(&find_ra);

    // Adjust the value in ra to point to the correct return location, 2nd
    // instruction past the real call into C code (the jalr(t9)), and push it.
    // This is the return address of the exit frame.
    const int kNumInstructionsToJump = 5;
    masm->Daddu(ra, ra, kNumInstructionsToJump * kInt32Size);
    masm->sd(ra, MemOperand(sp));  // This spot was reserved in EnterExitFrame.
    // Stack space reservation moved to the branch delay slot below.
    // Stack is still aligned.

    // Call the C routine.
    masm->mov(t9, s2);  // Function pointer to t9 to conform to ABI for PIC.
    masm->jalr(t9);
    // Set up sp in the delay slot.
    masm->daddiu(sp, sp, -kCArgsSlotsSize);
    // Make sure the stored 'ra' points to this position.
    DCHECK_EQ(kNumInstructionsToJump,
              masm->InstructionsGeneratedSince(&find_ra));
  }

  // Runtime functions should not return 'the hole'.  Allowing it to escape may
  // lead to crashes in the IC code later.
  if (FLAG_debug_code) {
    Label okay;
    __ LoadRoot(a4, Heap::kTheHoleValueRootIndex);
    __ Branch(&okay, ne, v0, Operand(a4));
    __ stop("The hole escaped");
    __ bind(&okay);
  }

  // Check result for exception sentinel.
  Label exception_returned;
  __ LoadRoot(a4, Heap::kExceptionRootIndex);
  __ Branch(&exception_returned, eq, a4, Operand(v0));

  ExternalReference pending_exception_address(
      Isolate::kPendingExceptionAddress, isolate());

  // Check that there is no pending exception, otherwise we
  // should have returned the exception sentinel.
  if (FLAG_debug_code) {
    Label okay;
    __ li(a2, Operand(pending_exception_address));
    __ ld(a2, MemOperand(a2));
    __ LoadRoot(a4, Heap::kTheHoleValueRootIndex);
    // Cannot use check here as it attempts to generate call into runtime.
    __ Branch(&okay, eq, a4, Operand(a2));
    __ stop("Unexpected pending exception");
    __ bind(&okay);
  }

  // Exit C frame and return.
  // v0:v1: result
  // sp: stack pointer
  // fp: frame pointer
  // s0: still holds argc (callee-saved).
  __ LeaveExitFrame(save_doubles(), s0, true, EMIT_RETURN);

  // Handling of exception.
  __ bind(&exception_returned);

  // Retrieve the pending exception.
  __ li(a2, Operand(pending_exception_address));
  __ ld(v0, MemOperand(a2));

  // Clear the pending exception.
  __ li(a3, Operand(isolate()->factory()->the_hole_value()));
  __ sd(a3, MemOperand(a2));

  // Special handling of termination exceptions which are uncatchable
  // by javascript code.
  Label throw_termination_exception;
  __ LoadRoot(a4, Heap::kTerminationExceptionRootIndex);
  __ Branch(&throw_termination_exception, eq, v0, Operand(a4));

  // Handle normal exception.
  __ Throw(v0);

  __ bind(&throw_termination_exception);
  __ ThrowUncatchable(v0);
}


void JSEntryStub::Generate(MacroAssembler* masm) {
  Label invoke, handler_entry, exit;
  Isolate* isolate = masm->isolate();

  // TODO(plind): unify the ABI description here.
  // Registers:
  // a0: entry address
  // a1: function
  // a2: receiver
  // a3: argc
  // a4 (a4): on mips64

  // Stack:
  // 0 arg slots on mips64 (4 args slots on mips)
  // args -- in a4/a4 on mips64, on stack on mips

  ProfileEntryHookStub::MaybeCallEntryHook(masm);

  // Save callee saved registers on the stack.
  __ MultiPush(kCalleeSaved | ra.bit());

  // Save callee-saved FPU registers.
  __ MultiPushFPU(kCalleeSavedFPU);
  // Set up the reserved register for 0.0.
  __ Move(kDoubleRegZero, 0.0);

  // Load argv in s0 register.
  if (kMipsAbi == kN64) {
    __ mov(s0, a4);  // 5th parameter in mips64 a4 (a4) register.
  } else {  // Abi O32.
    // 5th parameter on stack for O32 abi.
    int offset_to_argv = (kNumCalleeSaved + 1) * kPointerSize;
    offset_to_argv += kNumCalleeSavedFPU * kDoubleSize;
    __ ld(s0, MemOperand(sp, offset_to_argv + kCArgsSlotsSize));
  }

  __ InitializeRootRegister();

  // We build an EntryFrame.
  __ li(a7, Operand(-1));  // Push a bad frame pointer to fail if it is used.
  int marker = type();
  __ li(a6, Operand(Smi::FromInt(marker)));
  __ li(a5, Operand(Smi::FromInt(marker)));
  ExternalReference c_entry_fp(Isolate::kCEntryFPAddress, isolate);
  __ li(a4, Operand(c_entry_fp));
  __ ld(a4, MemOperand(a4));
  __ Push(a7, a6, a5, a4);
  // Set up frame pointer for the frame to be pushed.
  __ daddiu(fp, sp, -EntryFrameConstants::kCallerFPOffset);

  // Registers:
  // a0: entry_address
  // a1: function
  // a2: receiver_pointer
  // a3: argc
  // s0: argv
  //
  // Stack:
  // caller fp          |
  // function slot      | entry frame
  // context slot       |
  // bad fp (0xff...f)  |
  // callee saved registers + ra
  // [ O32: 4 args slots]
  // args

  // If this is the outermost JS call, set js_entry_sp value.
  Label non_outermost_js;
  ExternalReference js_entry_sp(Isolate::kJSEntrySPAddress, isolate);
  __ li(a5, Operand(ExternalReference(js_entry_sp)));
  __ ld(a6, MemOperand(a5));
  __ Branch(&non_outermost_js, ne, a6, Operand(zero_reg));
  __ sd(fp, MemOperand(a5));
  __ li(a4, Operand(Smi::FromInt(StackFrame::OUTERMOST_JSENTRY_FRAME)));
  Label cont;
  __ b(&cont);
  __ nop();   // Branch delay slot nop.
  __ bind(&non_outermost_js);
  __ li(a4, Operand(Smi::FromInt(StackFrame::INNER_JSENTRY_FRAME)));
  __ bind(&cont);
  __ push(a4);

  // Jump to a faked try block that does the invoke, with a faked catch
  // block that sets the pending exception.
  __ jmp(&invoke);
  __ bind(&handler_entry);
  handler_offset_ = handler_entry.pos();
  // Caught exception: Store result (exception) in the pending exception
  // field in the JSEnv and return a failure sentinel.  Coming in here the
  // fp will be invalid because the PushTryHandler below sets it to 0 to
  // signal the existence of the JSEntry frame.
  __ li(a4, Operand(ExternalReference(Isolate::kPendingExceptionAddress,
                                      isolate)));
  __ sd(v0, MemOperand(a4));  // We come back from 'invoke'. result is in v0.
  __ LoadRoot(v0, Heap::kExceptionRootIndex);
  __ b(&exit);  // b exposes branch delay slot.
  __ nop();   // Branch delay slot nop.

  // Invoke: Link this frame into the handler chain.  There's only one
  // handler block in this code object, so its index is 0.
  __ bind(&invoke);
  __ PushTryHandler(StackHandler::JS_ENTRY, 0);
  // If an exception not caught by another handler occurs, this handler
  // returns control to the code after the bal(&invoke) above, which
  // restores all kCalleeSaved registers (including cp and fp) to their
  // saved values before returning a failure to C.

  // Clear any pending exceptions.
  __ LoadRoot(a5, Heap::kTheHoleValueRootIndex);
  __ li(a4, Operand(ExternalReference(Isolate::kPendingExceptionAddress,
                                      isolate)));
  __ sd(a5, MemOperand(a4));

  // Invoke the function by calling through JS entry trampoline builtin.
  // Notice that we cannot store a reference to the trampoline code directly in
  // this stub, because runtime stubs are not traversed when doing GC.

  // Registers:
  // a0: entry_address
  // a1: function
  // a2: receiver_pointer
  // a3: argc
  // s0: argv
  //
  // Stack:
  // handler frame
  // entry frame
  // callee saved registers + ra
  // [ O32: 4 args slots]
  // args

  if (type() == StackFrame::ENTRY_CONSTRUCT) {
    ExternalReference construct_entry(Builtins::kJSConstructEntryTrampoline,
                                      isolate);
    __ li(a4, Operand(construct_entry));
  } else {
    ExternalReference entry(Builtins::kJSEntryTrampoline, masm->isolate());
    __ li(a4, Operand(entry));
  }
  __ ld(t9, MemOperand(a4));  // Deref address.
  // Call JSEntryTrampoline.
  __ daddiu(t9, t9, Code::kHeaderSize - kHeapObjectTag);
  __ Call(t9);

  // Unlink this frame from the handler chain.
  __ PopTryHandler();

  __ bind(&exit);  // v0 holds result
  // Check if the current stack frame is marked as the outermost JS frame.
  Label non_outermost_js_2;
  __ pop(a5);
  __ Branch(&non_outermost_js_2,
            ne,
            a5,
            Operand(Smi::FromInt(StackFrame::OUTERMOST_JSENTRY_FRAME)));
  __ li(a5, Operand(ExternalReference(js_entry_sp)));
  __ sd(zero_reg, MemOperand(a5));
  __ bind(&non_outermost_js_2);

  // Restore the top frame descriptors from the stack.
  __ pop(a5);
  __ li(a4, Operand(ExternalReference(Isolate::kCEntryFPAddress,
                                      isolate)));
  __ sd(a5, MemOperand(a4));

  // Reset the stack to the callee saved registers.
  __ daddiu(sp, sp, -EntryFrameConstants::kCallerFPOffset);

  // Restore callee-saved fpu registers.
  __ MultiPopFPU(kCalleeSavedFPU);

  // Restore callee saved registers from the stack.
  __ MultiPop(kCalleeSaved | ra.bit());
  // Return.
  __ Jump(ra);
}


void LoadIndexedStringStub::Generate(MacroAssembler* masm) {
  // Return address is in ra.
  Label miss;

  Register receiver = LoadDescriptor::ReceiverRegister();
  Register index = LoadDescriptor::NameRegister();
  Register scratch = a3;
  Register result = v0;
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


// Uses registers a0 to a4.
// Expected input (depending on whether args are in registers or on the stack):
// * object: a0 or at sp + 1 * kPointerSize.
// * function: a1 or at sp.
//
// An inlined call site may have been generated before calling this stub.
// In this case the offset to the inline site to patch is passed on the stack,
// in the safepoint slot for register a4.
void InstanceofStub::Generate(MacroAssembler* masm) {
  // Call site inlining and patching implies arguments in registers.
  DCHECK(HasArgsInRegisters() || !HasCallSiteInlineCheck());
  // ReturnTrueFalse is only implemented for inlined call sites.
  DCHECK(!ReturnTrueFalseObject() || HasCallSiteInlineCheck());

  // Fixed register usage throughout the stub:
  const Register object = a0;  // Object (lhs).
  Register map = a3;  // Map of the object.
  const Register function = a1;  // Function (rhs).
  const Register prototype = a4;  // Prototype of the function.
  const Register inline_site = t1;
  const Register scratch = a2;

  const int32_t kDeltaToLoadBoolResult = 7 * Assembler::kInstrSize;

  Label slow, loop, is_instance, is_not_instance, not_js_object;

  if (!HasArgsInRegisters()) {
    __ ld(object, MemOperand(sp, 1 * kPointerSize));
    __ ld(function, MemOperand(sp, 0));
  }

  // Check that the left hand is a JS object and load map.
  __ JumpIfSmi(object, &not_js_object);
  __ IsObjectJSObjectType(object, map, scratch, &not_js_object);

  // If there is a call site cache don't look in the global cache, but do the
  // real lookup and update the call site cache.
  if (!HasCallSiteInlineCheck()) {
    Label miss;
    __ LoadRoot(at, Heap::kInstanceofCacheFunctionRootIndex);
    __ Branch(&miss, ne, function, Operand(at));
    __ LoadRoot(at, Heap::kInstanceofCacheMapRootIndex);
    __ Branch(&miss, ne, map, Operand(at));
    __ LoadRoot(v0, Heap::kInstanceofCacheAnswerRootIndex);
    __ DropAndRet(HasArgsInRegisters() ? 0 : 2);

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

    // The offset was stored in a4 safepoint slot.
    // (See LCodeGen::DoDeferredLInstanceOfKnownGlobal).
    __ LoadFromSafepointRegisterSlot(scratch, a4);
    __ Dsubu(inline_site, ra, scratch);
    // Get the map location in scratch and patch it.
    __ GetRelocatedValue(inline_site, scratch, v1);  // v1 used as scratch.
    __ sd(map, FieldMemOperand(scratch, Cell::kValueOffset));
  }

  // Register mapping: a3 is object map and a4 is function prototype.
  // Get prototype of object into a2.
  __ ld(scratch, FieldMemOperand(map, Map::kPrototypeOffset));

  // We don't need map any more. Use it as a scratch register.
  Register scratch2 = map;
  map = no_reg;

  // Loop through the prototype chain looking for the function prototype.
  __ LoadRoot(scratch2, Heap::kNullValueRootIndex);
  __ bind(&loop);
  __ Branch(&is_instance, eq, scratch, Operand(prototype));
  __ Branch(&is_not_instance, eq, scratch, Operand(scratch2));
  __ ld(scratch, FieldMemOperand(scratch, HeapObject::kMapOffset));
  __ ld(scratch, FieldMemOperand(scratch, Map::kPrototypeOffset));
  __ Branch(&loop);

  __ bind(&is_instance);
  DCHECK(Smi::FromInt(0) == 0);
  if (!HasCallSiteInlineCheck()) {
    __ mov(v0, zero_reg);
    __ StoreRoot(v0, Heap::kInstanceofCacheAnswerRootIndex);
  } else {
    // Patch the call site to return true.
    __ LoadRoot(v0, Heap::kTrueValueRootIndex);
    __ Daddu(inline_site, inline_site, Operand(kDeltaToLoadBoolResult));
    // Get the boolean result location in scratch and patch it.
    __ PatchRelocatedValue(inline_site, scratch, v0);

    if (!ReturnTrueFalseObject()) {
      DCHECK_EQ(Smi::FromInt(0), 0);
      __ mov(v0, zero_reg);
    }
  }
  __ DropAndRet(HasArgsInRegisters() ? 0 : 2);

  __ bind(&is_not_instance);
  if (!HasCallSiteInlineCheck()) {
    __ li(v0, Operand(Smi::FromInt(1)));
    __ StoreRoot(v0, Heap::kInstanceofCacheAnswerRootIndex);
  } else {
    // Patch the call site to return false.
    __ LoadRoot(v0, Heap::kFalseValueRootIndex);
    __ Daddu(inline_site, inline_site, Operand(kDeltaToLoadBoolResult));
    // Get the boolean result location in scratch and patch it.
    __ PatchRelocatedValue(inline_site, scratch, v0);

    if (!ReturnTrueFalseObject()) {
      __ li(v0, Operand(Smi::FromInt(1)));
    }
  }

  __ DropAndRet(HasArgsInRegisters() ? 0 : 2);

  Label object_not_null, object_not_null_or_smi;
  __ bind(&not_js_object);
  // Before null, smi and string value checks, check that the rhs is a function
  // as for a non-function rhs an exception needs to be thrown.
  __ JumpIfSmi(function, &slow);
  __ GetObjectType(function, scratch2, scratch);
  __ Branch(&slow, ne, scratch, Operand(JS_FUNCTION_TYPE));

  // Null is not instance of anything.
  __ Branch(&object_not_null, ne, object,
            Operand(isolate()->factory()->null_value()));
  __ li(v0, Operand(Smi::FromInt(1)));
  __ DropAndRet(HasArgsInRegisters() ? 0 : 2);

  __ bind(&object_not_null);
  // Smi values are not instances of anything.
  __ JumpIfNotSmi(object, &object_not_null_or_smi);
  __ li(v0, Operand(Smi::FromInt(1)));
  __ DropAndRet(HasArgsInRegisters() ? 0 : 2);

  __ bind(&object_not_null_or_smi);
  // String values are not instances of anything.
  __ IsObjectJSStringType(object, scratch, &slow);
  __ li(v0, Operand(Smi::FromInt(1)));
  __ DropAndRet(HasArgsInRegisters() ? 0 : 2);

  // Slow-case.  Tail call builtin.
  __ bind(&slow);
  if (!ReturnTrueFalseObject()) {
    if (HasArgsInRegisters()) {
      __ Push(a0, a1);
    }
  __ InvokeBuiltin(Builtins::INSTANCE_OF, JUMP_FUNCTION);
  } else {
    {
      FrameScope scope(masm, StackFrame::INTERNAL);
      __ Push(a0, a1);
      __ InvokeBuiltin(Builtins::INSTANCE_OF, CALL_FUNCTION);
    }
    __ mov(a0, v0);
    __ LoadRoot(v0, Heap::kTrueValueRootIndex);
    __ DropAndRet(HasArgsInRegisters() ? 0 : 2, eq, a0, Operand(zero_reg));
    __ LoadRoot(v0, Heap::kFalseValueRootIndex);
    __ DropAndRet(HasArgsInRegisters() ? 0 : 2);
  }
}


void FunctionPrototypeStub::Generate(MacroAssembler* masm) {
  Label miss;
  Register receiver = LoadDescriptor::ReceiverRegister();
  NamedLoadHandlerCompiler::GenerateLoadFunctionPrototype(masm, receiver, a3,
                                                          a4, &miss);
  __ bind(&miss);
  PropertyAccessCompiler::TailCallBuiltin(
      masm, PropertyAccessCompiler::MissBuiltin(Code::LOAD_IC));
}


void ArgumentsAccessStub::GenerateReadElement(MacroAssembler* masm) {
  // The displacement is the offset of the last parameter (if any)
  // relative to the frame pointer.
  const int kDisplacement =
      StandardFrameConstants::kCallerSPOffset - kPointerSize;
  DCHECK(a1.is(ArgumentsAccessReadDescriptor::index()));
  DCHECK(a0.is(ArgumentsAccessReadDescriptor::parameter_count()));

  // Check that the key is a smiGenerateReadElement.
  Label slow;
  __ JumpIfNotSmi(a1, &slow);

  // Check if the calling frame is an arguments adaptor frame.
  Label adaptor;
  __ ld(a2, MemOperand(fp, StandardFrameConstants::kCallerFPOffset));
  __ ld(a3, MemOperand(a2, StandardFrameConstants::kContextOffset));
  __ Branch(&adaptor,
            eq,
            a3,
            Operand(Smi::FromInt(StackFrame::ARGUMENTS_ADAPTOR)));

  // Check index (a1) against formal parameters count limit passed in
  // through register a0. Use unsigned comparison to get negative
  // check for free.
  __ Branch(&slow, hs, a1, Operand(a0));

  // Read the argument from the stack and return it.
  __ dsubu(a3, a0, a1);
  __ SmiScale(a7, a3, kPointerSizeLog2);
  __ Daddu(a3, fp, Operand(a7));
  __ Ret(USE_DELAY_SLOT);
  __ ld(v0, MemOperand(a3, kDisplacement));

  // Arguments adaptor case: Check index (a1) against actual arguments
  // limit found in the arguments adaptor frame. Use unsigned
  // comparison to get negative check for free.
  __ bind(&adaptor);
  __ ld(a0, MemOperand(a2, ArgumentsAdaptorFrameConstants::kLengthOffset));
  __ Branch(&slow, Ugreater_equal, a1, Operand(a0));

  // Read the argument from the adaptor frame and return it.
  __ dsubu(a3, a0, a1);
  __ SmiScale(a7, a3, kPointerSizeLog2);
  __ Daddu(a3, a2, Operand(a7));
  __ Ret(USE_DELAY_SLOT);
  __ ld(v0, MemOperand(a3, kDisplacement));

  // Slow-case: Handle non-smi or out-of-bounds access to arguments
  // by calling the runtime system.
  __ bind(&slow);
  __ push(a1);
  __ TailCallRuntime(Runtime::kGetArgumentsProperty, 1, 1);
}


void ArgumentsAccessStub::GenerateNewSloppySlow(MacroAssembler* masm) {
  // sp[0] : number of parameters
  // sp[4] : receiver displacement
  // sp[8] : function
  // Check if the calling frame is an arguments adaptor frame.
  Label runtime;
  __ ld(a3, MemOperand(fp, StandardFrameConstants::kCallerFPOffset));
  __ ld(a2, MemOperand(a3, StandardFrameConstants::kContextOffset));
  __ Branch(&runtime,
            ne,
            a2,
            Operand(Smi::FromInt(StackFrame::ARGUMENTS_ADAPTOR)));

  // Patch the arguments.length and the parameters pointer in the current frame.
  __ ld(a2, MemOperand(a3, ArgumentsAdaptorFrameConstants::kLengthOffset));
  __ sd(a2, MemOperand(sp, 0 * kPointerSize));
  __ SmiScale(a7, a2, kPointerSizeLog2);
  __ Daddu(a3, a3, Operand(a7));
  __ daddiu(a3, a3, StandardFrameConstants::kCallerSPOffset);
  __ sd(a3, MemOperand(sp, 1 * kPointerSize));

  __ bind(&runtime);
  __ TailCallRuntime(Runtime::kNewSloppyArguments, 3, 1);
}


void ArgumentsAccessStub::GenerateNewSloppyFast(MacroAssembler* masm) {
  // Stack layout:
  //  sp[0] : number of parameters (tagged)
  //  sp[4] : address of receiver argument
  //  sp[8] : function
  // Registers used over whole function:
  //  a6 : allocated object (tagged)
  //  t1 : mapped parameter count (tagged)

  __ ld(a1, MemOperand(sp, 0 * kPointerSize));
  // a1 = parameter count (tagged)

  // Check if the calling frame is an arguments adaptor frame.
  Label runtime;
  Label adaptor_frame, try_allocate;
  __ ld(a3, MemOperand(fp, StandardFrameConstants::kCallerFPOffset));
  __ ld(a2, MemOperand(a3, StandardFrameConstants::kContextOffset));
  __ Branch(&adaptor_frame,
            eq,
            a2,
            Operand(Smi::FromInt(StackFrame::ARGUMENTS_ADAPTOR)));

  // No adaptor, parameter count = argument count.
  __ mov(a2, a1);
  __ Branch(&try_allocate);

  // We have an adaptor frame. Patch the parameters pointer.
  __ bind(&adaptor_frame);
  __ ld(a2, MemOperand(a3, ArgumentsAdaptorFrameConstants::kLengthOffset));
  __ SmiScale(t2, a2, kPointerSizeLog2);
  __ Daddu(a3, a3, Operand(t2));
  __ Daddu(a3, a3, Operand(StandardFrameConstants::kCallerSPOffset));
  __ sd(a3, MemOperand(sp, 1 * kPointerSize));

  // a1 = parameter count (tagged)
  // a2 = argument count (tagged)
  // Compute the mapped parameter count = min(a1, a2) in a1.
  Label skip_min;
  __ Branch(&skip_min, lt, a1, Operand(a2));
  __ mov(a1, a2);
  __ bind(&skip_min);

  __ bind(&try_allocate);

  // Compute the sizes of backing store, parameter map, and arguments object.
  // 1. Parameter map, has 2 extra words containing context and backing store.
  const int kParameterMapHeaderSize =
      FixedArray::kHeaderSize + 2 * kPointerSize;
  // If there are no mapped parameters, we do not need the parameter_map.
  Label param_map_size;
  DCHECK_EQ(0, Smi::FromInt(0));
  __ Branch(USE_DELAY_SLOT, &param_map_size, eq, a1, Operand(zero_reg));
  __ mov(t1, zero_reg);  // In delay slot: param map size = 0 when a1 == 0.
  __ SmiScale(t1, a1, kPointerSizeLog2);
  __ daddiu(t1, t1, kParameterMapHeaderSize);
  __ bind(&param_map_size);

  // 2. Backing store.
  __ SmiScale(t2, a2, kPointerSizeLog2);
  __ Daddu(t1, t1, Operand(t2));
  __ Daddu(t1, t1, Operand(FixedArray::kHeaderSize));

  // 3. Arguments object.
  __ Daddu(t1, t1, Operand(Heap::kSloppyArgumentsObjectSize));

  // Do the allocation of all three objects in one go.
  __ Allocate(t1, v0, a3, a4, &runtime, TAG_OBJECT);

  // v0 = address of new object(s) (tagged)
  // a2 = argument count (smi-tagged)
  // Get the arguments boilerplate from the current native context into a4.
  const int kNormalOffset =
      Context::SlotOffset(Context::SLOPPY_ARGUMENTS_MAP_INDEX);
  const int kAliasedOffset =
      Context::SlotOffset(Context::ALIASED_ARGUMENTS_MAP_INDEX);

  __ ld(a4, MemOperand(cp, Context::SlotOffset(Context::GLOBAL_OBJECT_INDEX)));
  __ ld(a4, FieldMemOperand(a4, GlobalObject::kNativeContextOffset));
  Label skip2_ne, skip2_eq;
  __ Branch(&skip2_ne, ne, a1, Operand(zero_reg));
  __ ld(a4, MemOperand(a4, kNormalOffset));
  __ bind(&skip2_ne);

  __ Branch(&skip2_eq, eq, a1, Operand(zero_reg));
  __ ld(a4, MemOperand(a4, kAliasedOffset));
  __ bind(&skip2_eq);

  // v0 = address of new object (tagged)
  // a1 = mapped parameter count (tagged)
  // a2 = argument count (smi-tagged)
  // a4 = address of arguments map (tagged)
  __ sd(a4, FieldMemOperand(v0, JSObject::kMapOffset));
  __ LoadRoot(a3, Heap::kEmptyFixedArrayRootIndex);
  __ sd(a3, FieldMemOperand(v0, JSObject::kPropertiesOffset));
  __ sd(a3, FieldMemOperand(v0, JSObject::kElementsOffset));

  // Set up the callee in-object property.
  STATIC_ASSERT(Heap::kArgumentsCalleeIndex == 1);
  __ ld(a3, MemOperand(sp, 2 * kPointerSize));
  __ AssertNotSmi(a3);
  const int kCalleeOffset = JSObject::kHeaderSize +
      Heap::kArgumentsCalleeIndex * kPointerSize;
  __ sd(a3, FieldMemOperand(v0, kCalleeOffset));

  // Use the length (smi tagged) and set that as an in-object property too.
  STATIC_ASSERT(Heap::kArgumentsLengthIndex == 0);
  const int kLengthOffset = JSObject::kHeaderSize +
      Heap::kArgumentsLengthIndex * kPointerSize;
  __ sd(a2, FieldMemOperand(v0, kLengthOffset));

  // Set up the elements pointer in the allocated arguments object.
  // If we allocated a parameter map, a4 will point there, otherwise
  // it will point to the backing store.
  __ Daddu(a4, v0, Operand(Heap::kSloppyArgumentsObjectSize));
  __ sd(a4, FieldMemOperand(v0, JSObject::kElementsOffset));

  // v0 = address of new object (tagged)
  // a1 = mapped parameter count (tagged)
  // a2 = argument count (tagged)
  // a4 = address of parameter map or backing store (tagged)
  // Initialize parameter map. If there are no mapped arguments, we're done.
  Label skip_parameter_map;
  Label skip3;
  __ Branch(&skip3, ne, a1, Operand(Smi::FromInt(0)));
  // Move backing store address to a3, because it is
  // expected there when filling in the unmapped arguments.
  __ mov(a3, a4);
  __ bind(&skip3);

  __ Branch(&skip_parameter_map, eq, a1, Operand(Smi::FromInt(0)));

  __ LoadRoot(a6, Heap::kSloppyArgumentsElementsMapRootIndex);
  __ sd(a6, FieldMemOperand(a4, FixedArray::kMapOffset));
  __ Daddu(a6, a1, Operand(Smi::FromInt(2)));
  __ sd(a6, FieldMemOperand(a4, FixedArray::kLengthOffset));
  __ sd(cp, FieldMemOperand(a4, FixedArray::kHeaderSize + 0 * kPointerSize));
  __ SmiScale(t2, a1, kPointerSizeLog2);
  __ Daddu(a6, a4, Operand(t2));
  __ Daddu(a6, a6, Operand(kParameterMapHeaderSize));
  __ sd(a6, FieldMemOperand(a4, FixedArray::kHeaderSize + 1 * kPointerSize));

  // Copy the parameter slots and the holes in the arguments.
  // We need to fill in mapped_parameter_count slots. They index the context,
  // where parameters are stored in reverse order, at
  //   MIN_CONTEXT_SLOTS .. MIN_CONTEXT_SLOTS+parameter_count-1
  // The mapped parameter thus need to get indices
  //   MIN_CONTEXT_SLOTS+parameter_count-1 ..
  //       MIN_CONTEXT_SLOTS+parameter_count-mapped_parameter_count
  // We loop from right to left.
  Label parameters_loop, parameters_test;
  __ mov(a6, a1);
  __ ld(t1, MemOperand(sp, 0 * kPointerSize));
  __ Daddu(t1, t1, Operand(Smi::FromInt(Context::MIN_CONTEXT_SLOTS)));
  __ Dsubu(t1, t1, Operand(a1));
  __ LoadRoot(a7, Heap::kTheHoleValueRootIndex);
  __ SmiScale(t2, a6, kPointerSizeLog2);
  __ Daddu(a3, a4, Operand(t2));
  __ Daddu(a3, a3, Operand(kParameterMapHeaderSize));

  // a6 = loop variable (tagged)
  // a1 = mapping index (tagged)
  // a3 = address of backing store (tagged)
  // a4 = address of parameter map (tagged)
  // a5 = temporary scratch (a.o., for address calculation)
  // a7 = the hole value
  __ jmp(&parameters_test);

  __ bind(&parameters_loop);

  __ Dsubu(a6, a6, Operand(Smi::FromInt(1)));
  __ SmiScale(a5, a6, kPointerSizeLog2);
  __ Daddu(a5, a5, Operand(kParameterMapHeaderSize - kHeapObjectTag));
  __ Daddu(t2, a4, a5);
  __ sd(t1, MemOperand(t2));
  __ Dsubu(a5, a5, Operand(kParameterMapHeaderSize - FixedArray::kHeaderSize));
  __ Daddu(t2, a3, a5);
  __ sd(a7, MemOperand(t2));
  __ Daddu(t1, t1, Operand(Smi::FromInt(1)));
  __ bind(&parameters_test);
  __ Branch(&parameters_loop, ne, a6, Operand(Smi::FromInt(0)));

  __ bind(&skip_parameter_map);
  // a2 = argument count (tagged)
  // a3 = address of backing store (tagged)
  // a5 = scratch
  // Copy arguments header and remaining slots (if there are any).
  __ LoadRoot(a5, Heap::kFixedArrayMapRootIndex);
  __ sd(a5, FieldMemOperand(a3, FixedArray::kMapOffset));
  __ sd(a2, FieldMemOperand(a3, FixedArray::kLengthOffset));

  Label arguments_loop, arguments_test;
  __ mov(t1, a1);
  __ ld(a4, MemOperand(sp, 1 * kPointerSize));
  __ SmiScale(t2, t1, kPointerSizeLog2);
  __ Dsubu(a4, a4, Operand(t2));
  __ jmp(&arguments_test);

  __ bind(&arguments_loop);
  __ Dsubu(a4, a4, Operand(kPointerSize));
  __ ld(a6, MemOperand(a4, 0));
  __ SmiScale(t2, t1, kPointerSizeLog2);
  __ Daddu(a5, a3, Operand(t2));
  __ sd(a6, FieldMemOperand(a5, FixedArray::kHeaderSize));
  __ Daddu(t1, t1, Operand(Smi::FromInt(1)));

  __ bind(&arguments_test);
  __ Branch(&arguments_loop, lt, t1, Operand(a2));

  // Return and remove the on-stack parameters.
  __ DropAndRet(3);

  // Do the runtime call to allocate the arguments object.
  // a2 = argument count (tagged)
  __ bind(&runtime);
  __ sd(a2, MemOperand(sp, 0 * kPointerSize));  // Patch argument count.
  __ TailCallRuntime(Runtime::kNewSloppyArguments, 3, 1);
}


void LoadIndexedInterceptorStub::Generate(MacroAssembler* masm) {
  // Return address is in ra.
  Label slow;

  Register receiver = LoadDescriptor::ReceiverRegister();
  Register key = LoadDescriptor::NameRegister();

  // Check that the key is an array index, that is Uint32.
  __ And(t0, key, Operand(kSmiTagMask | kSmiSignMask));
  __ Branch(&slow, ne, t0, Operand(zero_reg));

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
  __ ld(a2, MemOperand(fp, StandardFrameConstants::kCallerFPOffset));
  __ ld(a3, MemOperand(a2, StandardFrameConstants::kContextOffset));
  __ Branch(&adaptor_frame,
            eq,
            a3,
            Operand(Smi::FromInt(StackFrame::ARGUMENTS_ADAPTOR)));

  // Get the length from the frame.
  __ ld(a1, MemOperand(sp, 0));
  __ Branch(&try_allocate);

  // Patch the arguments.length and the parameters pointer.
  __ bind(&adaptor_frame);
  __ ld(a1, MemOperand(a2, ArgumentsAdaptorFrameConstants::kLengthOffset));
  __ sd(a1, MemOperand(sp, 0));
  __ SmiScale(at, a1, kPointerSizeLog2);

  __ Daddu(a3, a2, Operand(at));

  __ Daddu(a3, a3, Operand(StandardFrameConstants::kCallerSPOffset));
  __ sd(a3, MemOperand(sp, 1 * kPointerSize));

  // Try the new space allocation. Start out with computing the size
  // of the arguments object and the elements array in words.
  Label add_arguments_object;
  __ bind(&try_allocate);
  __ Branch(&add_arguments_object, eq, a1, Operand(zero_reg));
  __ SmiUntag(a1);

  __ Daddu(a1, a1, Operand(FixedArray::kHeaderSize / kPointerSize));
  __ bind(&add_arguments_object);
  __ Daddu(a1, a1, Operand(Heap::kStrictArgumentsObjectSize / kPointerSize));

  // Do the allocation of both objects in one go.
  __ Allocate(a1, v0, a2, a3, &runtime,
              static_cast<AllocationFlags>(TAG_OBJECT | SIZE_IN_WORDS));

  // Get the arguments boilerplate from the current native context.
  __ ld(a4, MemOperand(cp, Context::SlotOffset(Context::GLOBAL_OBJECT_INDEX)));
  __ ld(a4, FieldMemOperand(a4, GlobalObject::kNativeContextOffset));
  __ ld(a4, MemOperand(a4, Context::SlotOffset(
      Context::STRICT_ARGUMENTS_MAP_INDEX)));

  __ sd(a4, FieldMemOperand(v0, JSObject::kMapOffset));
  __ LoadRoot(a3, Heap::kEmptyFixedArrayRootIndex);
  __ sd(a3, FieldMemOperand(v0, JSObject::kPropertiesOffset));
  __ sd(a3, FieldMemOperand(v0, JSObject::kElementsOffset));

  // Get the length (smi tagged) and set that as an in-object property too.
  STATIC_ASSERT(Heap::kArgumentsLengthIndex == 0);
  __ ld(a1, MemOperand(sp, 0 * kPointerSize));
  __ AssertSmi(a1);
  __ sd(a1, FieldMemOperand(v0, JSObject::kHeaderSize +
      Heap::kArgumentsLengthIndex * kPointerSize));

  Label done;
  __ Branch(&done, eq, a1, Operand(zero_reg));

  // Get the parameters pointer from the stack.
  __ ld(a2, MemOperand(sp, 1 * kPointerSize));

  // Set up the elements pointer in the allocated arguments object and
  // initialize the header in the elements fixed array.
  __ Daddu(a4, v0, Operand(Heap::kStrictArgumentsObjectSize));
  __ sd(a4, FieldMemOperand(v0, JSObject::kElementsOffset));
  __ LoadRoot(a3, Heap::kFixedArrayMapRootIndex);
  __ sd(a3, FieldMemOperand(a4, FixedArray::kMapOffset));
  __ sd(a1, FieldMemOperand(a4, FixedArray::kLengthOffset));
  // Untag the length for the loop.
  __ SmiUntag(a1);


  // Copy the fixed array slots.
  Label loop;
  // Set up a4 to point to the first array slot.
  __ Daddu(a4, a4, Operand(FixedArray::kHeaderSize - kHeapObjectTag));
  __ bind(&loop);
  // Pre-decrement a2 with kPointerSize on each iteration.
  // Pre-decrement in order to skip receiver.
  __ Daddu(a2, a2, Operand(-kPointerSize));
  __ ld(a3, MemOperand(a2));
  // Post-increment a4 with kPointerSize on each iteration.
  __ sd(a3, MemOperand(a4));
  __ Daddu(a4, a4, Operand(kPointerSize));
  __ Dsubu(a1, a1, Operand(1));
  __ Branch(&loop, ne, a1, Operand(zero_reg));

  // Return and remove the on-stack parameters.
  __ bind(&done);
  __ DropAndRet(3);

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
  // MIPS - using s0..s2, since we are not using CEntry Stub.
  Register subject = s0;
  Register regexp_data = s1;
  Register last_match_info_elements = s2;

  // Ensure that a RegExp stack is allocated.
  ExternalReference address_of_regexp_stack_memory_address =
      ExternalReference::address_of_regexp_stack_memory_address(
          isolate());
  ExternalReference address_of_regexp_stack_memory_size =
      ExternalReference::address_of_regexp_stack_memory_size(isolate());
  __ li(a0, Operand(address_of_regexp_stack_memory_size));
  __ ld(a0, MemOperand(a0, 0));
  __ Branch(&runtime, eq, a0, Operand(zero_reg));

  // Check that the first argument is a JSRegExp object.
  __ ld(a0, MemOperand(sp, kJSRegExpOffset));
  STATIC_ASSERT(kSmiTag == 0);
  __ JumpIfSmi(a0, &runtime);
  __ GetObjectType(a0, a1, a1);
  __ Branch(&runtime, ne, a1, Operand(JS_REGEXP_TYPE));

  // Check that the RegExp has been compiled (data contains a fixed array).
  __ ld(regexp_data, FieldMemOperand(a0, JSRegExp::kDataOffset));
  if (FLAG_debug_code) {
    __ SmiTst(regexp_data, a4);
    __ Check(nz,
             kUnexpectedTypeForRegExpDataFixedArrayExpected,
             a4,
             Operand(zero_reg));
    __ GetObjectType(regexp_data, a0, a0);
    __ Check(eq,
             kUnexpectedTypeForRegExpDataFixedArrayExpected,
             a0,
             Operand(FIXED_ARRAY_TYPE));
  }

  // regexp_data: RegExp data (FixedArray)
  // Check the type of the RegExp. Only continue if type is JSRegExp::IRREGEXP.
  __ ld(a0, FieldMemOperand(regexp_data, JSRegExp::kDataTagOffset));
  __ Branch(&runtime, ne, a0, Operand(Smi::FromInt(JSRegExp::IRREGEXP)));

  // regexp_data: RegExp data (FixedArray)
  // Check that the number of captures fit in the static offsets vector buffer.
  __ ld(a2,
         FieldMemOperand(regexp_data, JSRegExp::kIrregexpCaptureCountOffset));
  // Check (number_of_captures + 1) * 2 <= offsets vector size
  // Or          number_of_captures * 2 <= offsets vector size - 2
  // Or          number_of_captures     <= offsets vector size / 2 - 1
  // Multiplying by 2 comes for free since a2 is smi-tagged.
  STATIC_ASSERT(Isolate::kJSRegexpStaticOffsetsVectorSize >= 2);
  int temp = Isolate::kJSRegexpStaticOffsetsVectorSize / 2 - 1;
  __ Branch(&runtime, hi, a2, Operand(Smi::FromInt(temp)));

  // Reset offset for possibly sliced string.
  __ mov(t0, zero_reg);
  __ ld(subject, MemOperand(sp, kSubjectOffset));
  __ JumpIfSmi(subject, &runtime);
  __ mov(a3, subject);  // Make a copy of the original subject string.
  __ ld(a0, FieldMemOperand(subject, HeapObject::kMapOffset));
  __ lbu(a0, FieldMemOperand(a0, Map::kInstanceTypeOffset));
  // subject: subject string
  // a3: subject string
  // a0: subject string instance type
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

  Label check_underlying;   // (4)
  Label seq_string;         // (5)
  Label not_seq_nor_cons;   // (6)
  Label external_string;    // (7)
  Label not_long_external;  // (8)

  // (1) Sequential string?  If yes, go to (5).
  __ And(a1,
         a0,
         Operand(kIsNotStringMask |
                 kStringRepresentationMask |
                 kShortExternalStringMask));
  STATIC_ASSERT((kStringTag | kSeqStringTag) == 0);
  __ Branch(&seq_string, eq, a1, Operand(zero_reg));  // Go to (5).

  // (2) Anything but sequential or cons?  If yes, go to (6).
  STATIC_ASSERT(kConsStringTag < kExternalStringTag);
  STATIC_ASSERT(kSlicedStringTag > kExternalStringTag);
  STATIC_ASSERT(kIsNotStringMask > kExternalStringTag);
  STATIC_ASSERT(kShortExternalStringTag > kExternalStringTag);
  // Go to (6).
  __ Branch(&not_seq_nor_cons, ge, a1, Operand(kExternalStringTag));

  // (3) Cons string.  Check that it's flat.
  // Replace subject with first string and reload instance type.
  __ ld(a0, FieldMemOperand(subject, ConsString::kSecondOffset));
  __ LoadRoot(a1, Heap::kempty_stringRootIndex);
  __ Branch(&runtime, ne, a0, Operand(a1));
  __ ld(subject, FieldMemOperand(subject, ConsString::kFirstOffset));

  // (4) Is subject external?  If yes, go to (7).
  __ bind(&check_underlying);
  __ ld(a0, FieldMemOperand(subject, HeapObject::kMapOffset));
  __ lbu(a0, FieldMemOperand(a0, Map::kInstanceTypeOffset));
  STATIC_ASSERT(kSeqStringTag == 0);
  __ And(at, a0, Operand(kStringRepresentationMask));
  // The underlying external string is never a short external string.
  STATIC_ASSERT(ExternalString::kMaxShortLength < ConsString::kMinLength);
  STATIC_ASSERT(ExternalString::kMaxShortLength < SlicedString::kMinLength);
  __ Branch(&external_string, ne, at, Operand(zero_reg));  // Go to (7).

  // (5) Sequential string.  Load regexp code according to encoding.
  __ bind(&seq_string);
  // subject: sequential subject string (or look-alike, external string)
  // a3: original subject string
  // Load previous index and check range before a3 is overwritten.  We have to
  // use a3 instead of subject here because subject might have been only made
  // to look like a sequential string when it actually is an external string.
  __ ld(a1, MemOperand(sp, kPreviousIndexOffset));
  __ JumpIfNotSmi(a1, &runtime);
  __ ld(a3, FieldMemOperand(a3, String::kLengthOffset));
  __ Branch(&runtime, ls, a3, Operand(a1));
  __ SmiUntag(a1);

  STATIC_ASSERT(kStringEncodingMask == 4);
  STATIC_ASSERT(kOneByteStringTag == 4);
  STATIC_ASSERT(kTwoByteStringTag == 0);
  __ And(a0, a0, Operand(kStringEncodingMask));  // Non-zero for one_byte.
  __ ld(t9, FieldMemOperand(regexp_data, JSRegExp::kDataOneByteCodeOffset));
  __ dsra(a3, a0, 2);  // a3 is 1 for one_byte, 0 for UC16 (used below).
  __ ld(a5, FieldMemOperand(regexp_data, JSRegExp::kDataUC16CodeOffset));
  __ Movz(t9, a5, a0);  // If UC16 (a0 is 0), replace t9 w/kDataUC16CodeOffset.

  // (E) Carry on.  String handling is done.
  // t9: irregexp code
  // Check that the irregexp code has been generated for the actual string
  // encoding. If it has, the field contains a code object otherwise it contains
  // a smi (code flushing support).
  __ JumpIfSmi(t9, &runtime);

  // a1: previous index
  // a3: encoding of subject string (1 if one_byte, 0 if two_byte);
  // t9: code
  // subject: Subject string
  // regexp_data: RegExp data (FixedArray)
  // All checks done. Now push arguments for native regexp code.
  __ IncrementCounter(isolate()->counters()->regexp_entry_native(),
                      1, a0, a2);

  // Isolates: note we add an additional parameter here (isolate pointer).
  const int kRegExpExecuteArguments = 9;
  const int kParameterRegisters = (kMipsAbi == kN64) ? 8 : 4;
  __ EnterExitFrame(false, kRegExpExecuteArguments - kParameterRegisters);

  // Stack pointer now points to cell where return address is to be written.
  // Arguments are before that on the stack or in registers, meaning we
  // treat the return address as argument 5. Thus every argument after that
  // needs to be shifted back by 1. Since DirectCEntryStub will handle
  // allocating space for the c argument slots, we don't need to calculate
  // that into the argument positions on the stack. This is how the stack will
  // look (sp meaning the value of sp at this moment):
  // Abi n64:
  //   [sp + 1] - Argument 9
  //   [sp + 0] - saved ra
  // Abi O32:
  //   [sp + 5] - Argument 9
  //   [sp + 4] - Argument 8
  //   [sp + 3] - Argument 7
  //   [sp + 2] - Argument 6
  //   [sp + 1] - Argument 5
  //   [sp + 0] - saved ra

  if (kMipsAbi == kN64) {
    // Argument 9: Pass current isolate address.
    __ li(a0, Operand(ExternalReference::isolate_address(isolate())));
    __ sd(a0, MemOperand(sp, 1 * kPointerSize));

    // Argument 8: Indicate that this is a direct call from JavaScript.
    __ li(a7, Operand(1));

    // Argument 7: Start (high end) of backtracking stack memory area.
    __ li(a0, Operand(address_of_regexp_stack_memory_address));
    __ ld(a0, MemOperand(a0, 0));
    __ li(a2, Operand(address_of_regexp_stack_memory_size));
    __ ld(a2, MemOperand(a2, 0));
    __ daddu(a6, a0, a2);

    // Argument 6: Set the number of capture registers to zero to force global
    // regexps to behave as non-global. This does not affect non-global regexps.
    __ mov(a5, zero_reg);

    // Argument 5: static offsets vector buffer.
    __ li(a4, Operand(
          ExternalReference::address_of_static_offsets_vector(isolate())));
  } else {  // O32.
    DCHECK(kMipsAbi == kO32);

    // Argument 9: Pass current isolate address.
    // CFunctionArgumentOperand handles MIPS stack argument slots.
    __ li(a0, Operand(ExternalReference::isolate_address(isolate())));
    __ sd(a0, MemOperand(sp, 5 * kPointerSize));

    // Argument 8: Indicate that this is a direct call from JavaScript.
    __ li(a0, Operand(1));
    __ sd(a0, MemOperand(sp, 4 * kPointerSize));

    // Argument 7: Start (high end) of backtracking stack memory area.
    __ li(a0, Operand(address_of_regexp_stack_memory_address));
    __ ld(a0, MemOperand(a0, 0));
    __ li(a2, Operand(address_of_regexp_stack_memory_size));
    __ ld(a2, MemOperand(a2, 0));
    __ daddu(a0, a0, a2);
    __ sd(a0, MemOperand(sp, 3 * kPointerSize));

    // Argument 6: Set the number of capture registers to zero to force global
    // regexps to behave as non-global. This does not affect non-global regexps.
    __ mov(a0, zero_reg);
    __ sd(a0, MemOperand(sp, 2 * kPointerSize));

    // Argument 5: static offsets vector buffer.
    __ li(a0, Operand(
          ExternalReference::address_of_static_offsets_vector(isolate())));
    __ sd(a0, MemOperand(sp, 1 * kPointerSize));
  }

  // For arguments 4 and 3 get string length, calculate start of string data
  // and calculate the shift of the index (0 for one_byte and 1 for two byte).
  __ Daddu(t2, subject, Operand(SeqString::kHeaderSize - kHeapObjectTag));
  __ Xor(a3, a3, Operand(1));  // 1 for 2-byte str, 0 for 1-byte.
  // Load the length from the original subject string from the previous stack
  // frame. Therefore we have to use fp, which points exactly to two pointer
  // sizes below the previous sp. (Because creating a new stack frame pushes
  // the previous fp onto the stack and moves up sp by 2 * kPointerSize.)
  __ ld(subject, MemOperand(fp, kSubjectOffset + 2 * kPointerSize));
  // If slice offset is not 0, load the length from the original sliced string.
  // Argument 4, a3: End of string data
  // Argument 3, a2: Start of string data
  // Prepare start and end index of the input.
  __ dsllv(t1, t0, a3);
  __ daddu(t0, t2, t1);
  __ dsllv(t1, a1, a3);
  __ daddu(a2, t0, t1);

  __ ld(t2, FieldMemOperand(subject, String::kLengthOffset));

  __ SmiUntag(t2);
  __ dsllv(t1, t2, a3);
  __ daddu(a3, t0, t1);
  // Argument 2 (a1): Previous index.
  // Already there

  // Argument 1 (a0): Subject string.
  __ mov(a0, subject);

  // Locate the code entry and call it.
  __ Daddu(t9, t9, Operand(Code::kHeaderSize - kHeapObjectTag));
  DirectCEntryStub stub(isolate());
  stub.GenerateCall(masm, t9);

  __ LeaveExitFrame(false, no_reg, true);

  // v0: result
  // subject: subject string (callee saved)
  // regexp_data: RegExp data (callee saved)
  // last_match_info_elements: Last match info elements (callee saved)
  // Check the result.
  Label success;
  __ Branch(&success, eq, v0, Operand(1));
  // We expect exactly one result since we force the called regexp to behave
  // as non-global.
  Label failure;
  __ Branch(&failure, eq, v0, Operand(NativeRegExpMacroAssembler::FAILURE));
  // If not exception it can only be retry. Handle that in the runtime system.
  __ Branch(&runtime, ne, v0, Operand(NativeRegExpMacroAssembler::EXCEPTION));
  // Result must now be exception. If there is no pending exception already a
  // stack overflow (on the backtrack stack) was detected in RegExp code but
  // haven't created the exception yet. Handle that in the runtime system.
  // TODO(592): Rerunning the RegExp to get the stack overflow exception.
  __ li(a1, Operand(isolate()->factory()->the_hole_value()));
  __ li(a2, Operand(ExternalReference(Isolate::kPendingExceptionAddress,
                                      isolate())));
  __ ld(v0, MemOperand(a2, 0));
  __ Branch(&runtime, eq, v0, Operand(a1));

  __ sd(a1, MemOperand(a2, 0));  // Clear pending exception.

  // Check if the exception is a termination. If so, throw as uncatchable.
  __ LoadRoot(a0, Heap::kTerminationExceptionRootIndex);
  Label termination_exception;
  __ Branch(&termination_exception, eq, v0, Operand(a0));

  __ Throw(v0);

  __ bind(&termination_exception);
  __ ThrowUncatchable(v0);

  __ bind(&failure);
  // For failure and exception return null.
  __ li(v0, Operand(isolate()->factory()->null_value()));
  __ DropAndRet(4);

  // Process the result from the native regexp code.
  __ bind(&success);

  __ lw(a1, UntagSmiFieldMemOperand(
      regexp_data, JSRegExp::kIrregexpCaptureCountOffset));
  // Calculate number of capture registers (number_of_captures + 1) * 2.
  __ Daddu(a1, a1, Operand(1));
  __ dsll(a1, a1, 1);  // Multiply by 2.

  __ ld(a0, MemOperand(sp, kLastMatchInfoOffset));
  __ JumpIfSmi(a0, &runtime);
  __ GetObjectType(a0, a2, a2);
  __ Branch(&runtime, ne, a2, Operand(JS_ARRAY_TYPE));
  // Check that the JSArray is in fast case.
  __ ld(last_match_info_elements,
        FieldMemOperand(a0, JSArray::kElementsOffset));
  __ ld(a0, FieldMemOperand(last_match_info_elements, HeapObject::kMapOffset));
  __ LoadRoot(at, Heap::kFixedArrayMapRootIndex);
  __ Branch(&runtime, ne, a0, Operand(at));
  // Check that the last match info has space for the capture registers and the
  // additional information.
  __ ld(a0,
        FieldMemOperand(last_match_info_elements, FixedArray::kLengthOffset));
  __ Daddu(a2, a1, Operand(RegExpImpl::kLastMatchOverhead));

  __ SmiUntag(at, a0);
  __ Branch(&runtime, gt, a2, Operand(at));

  // a1: number of capture registers
  // subject: subject string
  // Store the capture count.
  __ SmiTag(a2, a1);  // To smi.
  __ sd(a2, FieldMemOperand(last_match_info_elements,
                             RegExpImpl::kLastCaptureCountOffset));
  // Store last subject and last input.
  __ sd(subject,
         FieldMemOperand(last_match_info_elements,
                         RegExpImpl::kLastSubjectOffset));
  __ mov(a2, subject);
  __ RecordWriteField(last_match_info_elements,
                      RegExpImpl::kLastSubjectOffset,
                      subject,
                      a7,
                      kRAHasNotBeenSaved,
                      kDontSaveFPRegs);
  __ mov(subject, a2);
  __ sd(subject,
         FieldMemOperand(last_match_info_elements,
                         RegExpImpl::kLastInputOffset));
  __ RecordWriteField(last_match_info_elements,
                      RegExpImpl::kLastInputOffset,
                      subject,
                      a7,
                      kRAHasNotBeenSaved,
                      kDontSaveFPRegs);

  // Get the static offsets vector filled by the native regexp code.
  ExternalReference address_of_static_offsets_vector =
      ExternalReference::address_of_static_offsets_vector(isolate());
  __ li(a2, Operand(address_of_static_offsets_vector));

  // a1: number of capture registers
  // a2: offsets vector
  Label next_capture, done;
  // Capture register counter starts from number of capture registers and
  // counts down until wrapping after zero.
  __ Daddu(a0,
         last_match_info_elements,
         Operand(RegExpImpl::kFirstCaptureOffset - kHeapObjectTag));
  __ bind(&next_capture);
  __ Dsubu(a1, a1, Operand(1));
  __ Branch(&done, lt, a1, Operand(zero_reg));
  // Read the value from the static offsets vector buffer.
  __ lw(a3, MemOperand(a2, 0));
  __ daddiu(a2, a2, kIntSize);
  // Store the smi value in the last match info.
  __ SmiTag(a3);
  __ sd(a3, MemOperand(a0, 0));
  __ Branch(&next_capture, USE_DELAY_SLOT);
  __ daddiu(a0, a0, kPointerSize);  // In branch delay slot.

  __ bind(&done);

  // Return last match info.
  __ ld(v0, MemOperand(sp, kLastMatchInfoOffset));
  __ DropAndRet(4);

  // Do the runtime call to execute the regexp.
  __ bind(&runtime);
  __ TailCallRuntime(Runtime::kRegExpExecRT, 4, 1);

  // Deferred code for string handling.
  // (6) Not a long external string?  If yes, go to (8).
  __ bind(&not_seq_nor_cons);
  // Go to (8).
  __ Branch(&not_long_external, gt, a1, Operand(kExternalStringTag));

  // (7) External string.  Make it, offset-wise, look like a sequential string.
  __ bind(&external_string);
  __ ld(a0, FieldMemOperand(subject, HeapObject::kMapOffset));
  __ lbu(a0, FieldMemOperand(a0, Map::kInstanceTypeOffset));
  if (FLAG_debug_code) {
    // Assert that we do not have a cons or slice (indirect strings) here.
    // Sequential strings have already been ruled out.
    __ And(at, a0, Operand(kIsIndirectStringMask));
    __ Assert(eq,
              kExternalStringExpectedButNotFound,
              at,
              Operand(zero_reg));
  }
  __ ld(subject,
        FieldMemOperand(subject, ExternalString::kResourceDataOffset));
  // Move the pointer so that offset-wise, it looks like a sequential string.
  STATIC_ASSERT(SeqTwoByteString::kHeaderSize == SeqOneByteString::kHeaderSize);
  __ Dsubu(subject,
          subject,
          SeqTwoByteString::kHeaderSize - kHeapObjectTag);
  __ jmp(&seq_string);    // Go to (5).

  // (8) Short external string or not a string?  If yes, bail out to runtime.
  __ bind(&not_long_external);
  STATIC_ASSERT(kNotStringTag != 0 && kShortExternalStringTag !=0);
  __ And(at, a1, Operand(kIsNotStringMask | kShortExternalStringMask));
  __ Branch(&runtime, ne, at, Operand(zero_reg));

  // (9) Sliced string.  Replace subject with parent.  Go to (4).
  // Load offset into t0 and replace subject string with parent.
  __ ld(t0, FieldMemOperand(subject, SlicedString::kOffsetOffset));
  __ SmiUntag(t0);
  __ ld(subject, FieldMemOperand(subject, SlicedString::kParentOffset));
  __ jmp(&check_underlying);  // Go to (4).
#endif  // V8_INTERPRETED_REGEXP
}


static void GenerateRecordCallTarget(MacroAssembler* masm) {
  // Cache the called function in a feedback vector slot.  Cache states
  // are uninitialized, monomorphic (indicated by a JSFunction), and
  // megamorphic.
  // a0 : number of arguments to the construct function
  // a1 : the function to call
  // a2 : Feedback vector
  // a3 : slot in feedback vector (Smi)
  Label initialize, done, miss, megamorphic, not_array_function;

  DCHECK_EQ(*TypeFeedbackVector::MegamorphicSentinel(masm->isolate()),
            masm->isolate()->heap()->megamorphic_symbol());
  DCHECK_EQ(*TypeFeedbackVector::UninitializedSentinel(masm->isolate()),
            masm->isolate()->heap()->uninitialized_symbol());

  // Load the cache state into a4.
  __ dsrl(a4, a3, 32 - kPointerSizeLog2);
  __ Daddu(a4, a2, Operand(a4));
  __ ld(a4, FieldMemOperand(a4, FixedArray::kHeaderSize));

  // A monomorphic cache hit or an already megamorphic state: invoke the
  // function without changing the state.
  __ Branch(&done, eq, a4, Operand(a1));

  if (!FLAG_pretenuring_call_new) {
    // If we came here, we need to see if we are the array function.
    // If we didn't have a matching function, and we didn't find the megamorph
    // sentinel, then we have in the slot either some other function or an
    // AllocationSite. Do a map check on the object in a3.
    __ ld(a5, FieldMemOperand(a4, 0));
    __ LoadRoot(at, Heap::kAllocationSiteMapRootIndex);
    __ Branch(&miss, ne, a5, Operand(at));

    // Make sure the function is the Array() function
    __ LoadGlobalFunction(Context::ARRAY_FUNCTION_INDEX, a4);
    __ Branch(&megamorphic, ne, a1, Operand(a4));
    __ jmp(&done);
  }

  __ bind(&miss);

  // A monomorphic miss (i.e, here the cache is not uninitialized) goes
  // megamorphic.
  __ LoadRoot(at, Heap::kuninitialized_symbolRootIndex);
  __ Branch(&initialize, eq, a4, Operand(at));
  // MegamorphicSentinel is an immortal immovable object (undefined) so no
  // write-barrier is needed.
  __ bind(&megamorphic);
  __ dsrl(a4, a3, 32- kPointerSizeLog2);
  __ Daddu(a4, a2, Operand(a4));
  __ LoadRoot(at, Heap::kmegamorphic_symbolRootIndex);
  __ sd(at, FieldMemOperand(a4, FixedArray::kHeaderSize));
  __ jmp(&done);

  // An uninitialized cache is patched with the function.
  __ bind(&initialize);
  if (!FLAG_pretenuring_call_new) {
    // Make sure the function is the Array() function.
    __ LoadGlobalFunction(Context::ARRAY_FUNCTION_INDEX, a4);
    __ Branch(&not_array_function, ne, a1, Operand(a4));

    // The target function is the Array constructor,
    // Create an AllocationSite if we don't already have it, store it in the
    // slot.
    {
      FrameScope scope(masm, StackFrame::INTERNAL);
      const RegList kSavedRegs =
          1 << 4  |  // a0
          1 << 5  |  // a1
          1 << 6  |  // a2
          1 << 7;    // a3

      // Arguments register must be smi-tagged to call out.
      __ SmiTag(a0);
      __ MultiPush(kSavedRegs);

      CreateAllocationSiteStub create_stub(masm->isolate());
      __ CallStub(&create_stub);

      __ MultiPop(kSavedRegs);
      __ SmiUntag(a0);
    }
    __ Branch(&done);

    __ bind(&not_array_function);
  }

  __ dsrl(a4, a3, 32 - kPointerSizeLog2);
  __ Daddu(a4, a2, Operand(a4));
  __ Daddu(a4, a4, Operand(FixedArray::kHeaderSize - kHeapObjectTag));
  __ sd(a1, MemOperand(a4, 0));

  __ Push(a4, a2, a1);
  __ RecordWrite(a2, a4, a1, kRAHasNotBeenSaved, kDontSaveFPRegs,
                 EMIT_REMEMBERED_SET, OMIT_SMI_CHECK);
  __ Pop(a4, a2, a1);

  __ bind(&done);
}


static void EmitContinueIfStrictOrNative(MacroAssembler* masm, Label* cont) {
  __ ld(a3, FieldMemOperand(a1, JSFunction::kSharedFunctionInfoOffset));

  // Do not transform the receiver for strict mode functions.
  int32_t strict_mode_function_mask =
      1 <<  SharedFunctionInfo::kStrictModeBitWithinByte ;
  // Do not transform the receiver for native (Compilerhints already in a3).
  int32_t native_mask = 1 << SharedFunctionInfo::kNativeBitWithinByte;

  __ lbu(a4, FieldMemOperand(a3, SharedFunctionInfo::kStrictModeByteOffset));
  __ And(at, a4, Operand(strict_mode_function_mask));
  __ Branch(cont, ne, at, Operand(zero_reg));
  __ lbu(a4, FieldMemOperand(a3, SharedFunctionInfo::kNativeByteOffset));
  __ And(at, a4, Operand(native_mask));
  __ Branch(cont, ne, at, Operand(zero_reg));
}


static void EmitSlowCase(MacroAssembler* masm,
                         int argc,
                         Label* non_function) {
  // Check for function proxy.
  __ Branch(non_function, ne, a4, Operand(JS_FUNCTION_PROXY_TYPE));
  __ push(a1);  // put proxy as additional argument
  __ li(a0, Operand(argc + 1, RelocInfo::NONE32));
  __ mov(a2, zero_reg);
  __ GetBuiltinFunction(a1, Builtins::CALL_FUNCTION_PROXY);
  {
    Handle<Code> adaptor =
        masm->isolate()->builtins()->ArgumentsAdaptorTrampoline();
    __ Jump(adaptor, RelocInfo::CODE_TARGET);
  }

  // CALL_NON_FUNCTION expects the non-function callee as receiver (instead
  // of the original receiver from the call site).
  __ bind(non_function);
  __ sd(a1, MemOperand(sp, argc * kPointerSize));
  __ li(a0, Operand(argc));  // Set up the number of arguments.
  __ mov(a2, zero_reg);
  __ GetBuiltinFunction(a1, Builtins::CALL_NON_FUNCTION);
  __ Jump(masm->isolate()->builtins()->ArgumentsAdaptorTrampoline(),
          RelocInfo::CODE_TARGET);
}


static void EmitWrapCase(MacroAssembler* masm, int argc, Label* cont) {
  // Wrap the receiver and patch it back onto the stack.
  { FrameScope frame_scope(masm, StackFrame::INTERNAL);
    __ Push(a1, a3);
    __ InvokeBuiltin(Builtins::TO_OBJECT, CALL_FUNCTION);
    __ pop(a1);
  }
  __ Branch(USE_DELAY_SLOT, cont);
  __ sd(v0, MemOperand(sp, argc * kPointerSize));
}


static void CallFunctionNoFeedback(MacroAssembler* masm,
                                   int argc, bool needs_checks,
                                   bool call_as_method) {
  // a1 : the function to call
  Label slow, non_function, wrap, cont;

  if (needs_checks) {
    // Check that the function is really a JavaScript function.
    // a1: pushed function (to be verified)
    __ JumpIfSmi(a1, &non_function);

    // Goto slow case if we do not have a function.
    __ GetObjectType(a1, a4, a4);
    __ Branch(&slow, ne, a4, Operand(JS_FUNCTION_TYPE));
  }

  // Fast-case: Invoke the function now.
  // a1: pushed function
  ParameterCount actual(argc);

  if (call_as_method) {
    if (needs_checks) {
      EmitContinueIfStrictOrNative(masm, &cont);
    }

    // Compute the receiver in sloppy mode.
    __ ld(a3, MemOperand(sp, argc * kPointerSize));

    if (needs_checks) {
      __ JumpIfSmi(a3, &wrap);
      __ GetObjectType(a3, a4, a4);
      __ Branch(&wrap, lt, a4, Operand(FIRST_SPEC_OBJECT_TYPE));
    } else {
      __ jmp(&wrap);
    }

    __ bind(&cont);
  }
  __ InvokeFunction(a1, actual, JUMP_FUNCTION, NullCallWrapper());

  if (needs_checks) {
    // Slow-case: Non-function called.
    __ bind(&slow);
    EmitSlowCase(masm, argc, &non_function);
  }

  if (call_as_method) {
    __ bind(&wrap);
    // Wrap the receiver and patch it back onto the stack.
    EmitWrapCase(masm, argc, &cont);
  }
}


void CallFunctionStub::Generate(MacroAssembler* masm) {
  CallFunctionNoFeedback(masm, argc(), NeedsChecks(), CallAsMethod());
}


void CallConstructStub::Generate(MacroAssembler* masm) {
  // a0 : number of arguments
  // a1 : the function to call
  // a2 : feedback vector
  // a3 : (only if a2 is not undefined) slot in feedback vector (Smi)
  Label slow, non_function_call;
  // Check that the function is not a smi.
  __ JumpIfSmi(a1, &non_function_call);
  // Check that the function is a JSFunction.
  __ GetObjectType(a1, a4, a4);
  __ Branch(&slow, ne, a4, Operand(JS_FUNCTION_TYPE));

  if (RecordCallTarget()) {
    GenerateRecordCallTarget(masm);

    __ dsrl(at, a3, 32 - kPointerSizeLog2);
    __ Daddu(a5, a2, at);
    if (FLAG_pretenuring_call_new) {
      // Put the AllocationSite from the feedback vector into a2.
      // By adding kPointerSize we encode that we know the AllocationSite
      // entry is at the feedback vector slot given by a3 + 1.
      __ ld(a2, FieldMemOperand(a5, FixedArray::kHeaderSize + kPointerSize));
    } else {
      Label feedback_register_initialized;
      // Put the AllocationSite from the feedback vector into a2, or undefined.
      __ ld(a2, FieldMemOperand(a5, FixedArray::kHeaderSize));
      __ ld(a5, FieldMemOperand(a2, AllocationSite::kMapOffset));
      __ LoadRoot(at, Heap::kAllocationSiteMapRootIndex);
      __ Branch(&feedback_register_initialized, eq, a5, Operand(at));
      __ LoadRoot(a2, Heap::kUndefinedValueRootIndex);
      __ bind(&feedback_register_initialized);
    }

    __ AssertUndefinedOrAllocationSite(a2, a5);
  }

  // Jump to the function-specific construct stub.
  Register jmp_reg = a4;
  __ ld(jmp_reg, FieldMemOperand(a1, JSFunction::kSharedFunctionInfoOffset));
  __ ld(jmp_reg, FieldMemOperand(jmp_reg,
                                 SharedFunctionInfo::kConstructStubOffset));
  __ Daddu(at, jmp_reg, Operand(Code::kHeaderSize - kHeapObjectTag));
  __ Jump(at);

  // a0: number of arguments
  // a1: called object
  // a4: object type
  Label do_call;
  __ bind(&slow);
  __ Branch(&non_function_call, ne, a4, Operand(JS_FUNCTION_PROXY_TYPE));
  __ GetBuiltinFunction(a1, Builtins::CALL_FUNCTION_PROXY_AS_CONSTRUCTOR);
  __ jmp(&do_call);

  __ bind(&non_function_call);
  __ GetBuiltinFunction(a1, Builtins::CALL_NON_FUNCTION_AS_CONSTRUCTOR);
  __ bind(&do_call);
  // Set expected number of arguments to zero (not changing r0).
  __ li(a2, Operand(0, RelocInfo::NONE32));
  __ Jump(masm->isolate()->builtins()->ArgumentsAdaptorTrampoline(),
           RelocInfo::CODE_TARGET);
}


// StringCharCodeAtGenerator.
void StringCharCodeAtGenerator::GenerateFast(MacroAssembler* masm) {
  DCHECK(!a4.is(index_));
  DCHECK(!a4.is(result_));
  DCHECK(!a4.is(object_));

  // If the receiver is a smi trigger the non-string case.
  if (check_mode_ == RECEIVER_IS_UNKNOWN) {
    __ JumpIfSmi(object_, receiver_not_string_);

    // Fetch the instance type of the receiver into result register.
    __ ld(result_, FieldMemOperand(object_, HeapObject::kMapOffset));
    __ lbu(result_, FieldMemOperand(result_, Map::kInstanceTypeOffset));
    // If the receiver is not a string trigger the non-string case.
    __ And(a4, result_, Operand(kIsNotStringMask));
    __ Branch(receiver_not_string_, ne, a4, Operand(zero_reg));
  }

  // If the index is non-smi trigger the non-smi case.
  __ JumpIfNotSmi(index_, &index_not_smi_);

  __ bind(&got_smi_index_);

  // Check for index out of range.
  __ ld(a4, FieldMemOperand(object_, String::kLengthOffset));
  __ Branch(index_out_of_range_, ls, a4, Operand(index_));

  __ SmiUntag(index_);

  StringCharLoadGenerator::Generate(masm,
                                    object_,
                                    index_,
                                    result_,
                                    &call_runtime_);

  __ SmiTag(result_);
  __ bind(&exit_);
}


static void EmitLoadTypeFeedbackVector(MacroAssembler* masm, Register vector) {
  __ ld(vector, MemOperand(fp, JavaScriptFrameConstants::kFunctionOffset));
  __ ld(vector, FieldMemOperand(vector,
                                JSFunction::kSharedFunctionInfoOffset));
  __ ld(vector, FieldMemOperand(vector,
                                SharedFunctionInfo::kFeedbackVectorOffset));
}


void CallIC_ArrayStub::Generate(MacroAssembler* masm) {
  // a1 - function
  // a3 - slot id
  Label miss;

  EmitLoadTypeFeedbackVector(masm, a2);

  __ LoadGlobalFunction(Context::ARRAY_FUNCTION_INDEX, at);
  __ Branch(&miss, ne, a1, Operand(at));

  __ li(a0, Operand(arg_count()));
  __ dsrl(at, a3, 32 - kPointerSizeLog2);
  __ Daddu(at, a2, Operand(at));
  __ ld(a4, FieldMemOperand(at, FixedArray::kHeaderSize));

  // Verify that a4 contains an AllocationSite
  __ ld(a5, FieldMemOperand(a4, HeapObject::kMapOffset));
  __ LoadRoot(at, Heap::kAllocationSiteMapRootIndex);
  __ Branch(&miss, ne, a5, Operand(at));

  __ mov(a2, a4);
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
  // a1 - function
  // a3 - slot id (Smi)
  Label extra_checks_or_miss, slow_start;
  Label slow, non_function, wrap, cont;
  Label have_js_function;
  int argc = arg_count();
  ParameterCount actual(argc);

  EmitLoadTypeFeedbackVector(masm, a2);

  // The checks. First, does r1 match the recorded monomorphic target?
  __ dsrl(a4, a3, 32 - kPointerSizeLog2);
  __ Daddu(a4, a2, Operand(a4));
  __ ld(a4, FieldMemOperand(a4, FixedArray::kHeaderSize));
  __ Branch(&extra_checks_or_miss, ne, a1, Operand(a4));

  __ bind(&have_js_function);
  if (CallAsMethod()) {
    EmitContinueIfStrictOrNative(masm, &cont);
    // Compute the receiver in sloppy mode.
    __ ld(a3, MemOperand(sp, argc * kPointerSize));

    __ JumpIfSmi(a3, &wrap);
    __ GetObjectType(a3, a4, a4);
    __ Branch(&wrap, lt, a4, Operand(FIRST_SPEC_OBJECT_TYPE));

    __ bind(&cont);
  }

  __ InvokeFunction(a1, actual, JUMP_FUNCTION, NullCallWrapper());

  __ bind(&slow);
  EmitSlowCase(masm, argc, &non_function);

  if (CallAsMethod()) {
    __ bind(&wrap);
    EmitWrapCase(masm, argc, &cont);
  }

  __ bind(&extra_checks_or_miss);
  Label miss;

  __ LoadRoot(at, Heap::kmegamorphic_symbolRootIndex);
  __ Branch(&slow_start, eq, a4, Operand(at));
  __ LoadRoot(at, Heap::kuninitialized_symbolRootIndex);
  __ Branch(&miss, eq, a4, Operand(at));

  if (!FLAG_trace_ic) {
    // We are going megamorphic. If the feedback is a JSFunction, it is fine
    // to handle it here. More complex cases are dealt with in the runtime.
    __ AssertNotSmi(a4);
    __ GetObjectType(a4, a5, a5);
    __ Branch(&miss, ne, a5, Operand(JS_FUNCTION_TYPE));
    __ dsrl(a4, a3, 32 - kPointerSizeLog2);
    __ Daddu(a4, a2, Operand(a4));
    __ LoadRoot(at, Heap::kmegamorphic_symbolRootIndex);
    __ sd(at, FieldMemOperand(a4, FixedArray::kHeaderSize));
    // We have to update statistics for runtime profiling.
    const int with_types_offset =
    FixedArray::OffsetOfElementAt(TypeFeedbackVector::kWithTypesIndex);
    __ ld(a4, FieldMemOperand(a2, with_types_offset));
    __ Dsubu(a4, a4, Operand(Smi::FromInt(1)));
    __ sd(a4, FieldMemOperand(a2, with_types_offset));
    const int generic_offset =
    FixedArray::OffsetOfElementAt(TypeFeedbackVector::kGenericCountIndex);
    __ ld(a4, FieldMemOperand(a2, generic_offset));
    __ Daddu(a4, a4, Operand(Smi::FromInt(1)));
    __ Branch(USE_DELAY_SLOT, &slow_start);
    __ sd(a4, FieldMemOperand(a2, generic_offset));  // In delay slot.
  }

  // We are here because tracing is on or we are going monomorphic.
  __ bind(&miss);
  GenerateMiss(masm);

  // the slow case
  __ bind(&slow_start);
  // Check that the function is really a JavaScript function.
  // r1: pushed function (to be verified)
  __ JumpIfSmi(a1, &non_function);

  // Goto slow case if we do not have a function.
  __ GetObjectType(a1, a4, a4);
  __ Branch(&slow, ne, a4, Operand(JS_FUNCTION_TYPE));
  __ Branch(&have_js_function);
}


void CallICStub::GenerateMiss(MacroAssembler* masm) {
  // Get the receiver of the function from the stack; 1 ~ return address.
  __ ld(a4, MemOperand(sp, (arg_count() + 1) * kPointerSize));

  {
    FrameScope scope(masm, StackFrame::INTERNAL);

    // Push the receiver and the function and feedback info.
    __ Push(a4, a1, a2, a3);

    // Call the entry.
    IC::UtilityId id = GetICState() == DEFAULT ? IC::kCallIC_Miss
                                               : IC::kCallIC_Customization_Miss;

    ExternalReference miss = ExternalReference(IC_Utility(id),
                                               masm->isolate());
    __ CallExternalReference(miss, 4);

    // Move result to a1 and exit the internal frame.
    __ mov(a1, v0);
  }
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
  // Consumed by runtime conversion function:
  __ Push(object_, index_);
  if (index_flags_ == STRING_INDEX_IS_NUMBER) {
    __ CallRuntime(Runtime::kNumberToIntegerMapMinusZero, 1);
  } else {
    DCHECK(index_flags_ == STRING_INDEX_IS_ARRAY_INDEX);
    // NumberToSmi discards numbers that are not exact integers.
    __ CallRuntime(Runtime::kNumberToSmi, 1);
  }

  // Save the conversion result before the pop instructions below
  // have a chance to overwrite it.

  __ Move(index_, v0);
  __ pop(object_);
  // Reload the instance type.
  __ ld(result_, FieldMemOperand(object_, HeapObject::kMapOffset));
  __ lbu(result_, FieldMemOperand(result_, Map::kInstanceTypeOffset));
  call_helper.AfterCall(masm);
  // If index is still not a smi, it must be out of range.
  __ JumpIfNotSmi(index_, index_out_of_range_);
  // Otherwise, return to the fast path.
  __ Branch(&got_smi_index_);

  // Call runtime. We get here when the receiver is a string and the
  // index is a number, but the code of getting the actual character
  // is too complex (e.g., when the string needs to be flattened).
  __ bind(&call_runtime_);
  call_helper.BeforeCall(masm);
  __ SmiTag(index_);
  __ Push(object_, index_);
  __ CallRuntime(Runtime::kStringCharCodeAtRT, 2);

  __ Move(result_, v0);

  call_helper.AfterCall(masm);
  __ jmp(&exit_);

  __ Abort(kUnexpectedFallthroughFromCharCodeAtSlowCase);
}


// -------------------------------------------------------------------------
// StringCharFromCodeGenerator

void StringCharFromCodeGenerator::GenerateFast(MacroAssembler* masm) {
  // Fast case of Heap::LookupSingleCharacterStringFromCode.

  DCHECK(!a4.is(result_));
  DCHECK(!a4.is(code_));

  STATIC_ASSERT(kSmiTag == 0);
  DCHECK(base::bits::IsPowerOfTwo32(String::kMaxOneByteCharCode + 1));
  __ And(a4,
         code_,
         Operand(kSmiTagMask |
                 ((~String::kMaxOneByteCharCode) << kSmiTagSize)));
  __ Branch(&slow_case_, ne, a4, Operand(zero_reg));


  __ LoadRoot(result_, Heap::kSingleCharacterStringCacheRootIndex);
  // At this point code register contains smi tagged one_byte char code.
  STATIC_ASSERT(kSmiTag == 0);
  __ SmiScale(a4, code_, kPointerSizeLog2);
  __ Daddu(result_, result_, a4);
  __ ld(result_, FieldMemOperand(result_, FixedArray::kHeaderSize));
  __ LoadRoot(a4, Heap::kUndefinedValueRootIndex);
  __ Branch(&slow_case_, eq, result_, Operand(a4));
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
  __ Move(result_, v0);

  call_helper.AfterCall(masm);
  __ Branch(&exit_);

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
    __ And(scratch, dest, Operand(kPointerAlignmentMask));
    __ Check(eq,
             kDestinationOfCopyNotAligned,
             scratch,
             Operand(zero_reg));
  }

  // Assumes word reads and writes are little endian.
  // Nothing to do for zero characters.
  Label done;

  if (encoding == String::TWO_BYTE_ENCODING) {
    __ Daddu(count, count, count);
  }

  Register limit = count;  // Read until dest equals this.
  __ Daddu(limit, dest, Operand(count));

  Label loop_entry, loop;
  // Copy bytes from src to dest until dest hits limit.
  __ Branch(&loop_entry);
  __ bind(&loop);
  __ lbu(scratch, MemOperand(src));
  __ daddiu(src, src, 1);
  __ sb(scratch, MemOperand(dest));
  __ daddiu(dest, dest, 1);
  __ bind(&loop_entry);
  __ Branch(&loop, lt, dest, Operand(limit));

  __ bind(&done);
}


void SubStringStub::Generate(MacroAssembler* masm) {
  Label runtime;
  // Stack frame on entry.
  //  ra: return address
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

  __ ld(a2, MemOperand(sp, kToOffset));
  __ ld(a3, MemOperand(sp, kFromOffset));
// Does not needed?
//  STATIC_ASSERT(kFromOffset == kToOffset + 4);
  STATIC_ASSERT(kSmiTag == 0);
// Does not needed?
// STATIC_ASSERT(kSmiTagSize + kSmiShiftSize == 1);

  // Utilize delay slots. SmiUntag doesn't emit a jump, everything else is
  // safe in this case.
  __ JumpIfNotSmi(a2, &runtime);
  __ JumpIfNotSmi(a3, &runtime);
  // Both a2 and a3 are untagged integers.

  __ SmiUntag(a2, a2);
  __ SmiUntag(a3, a3);
  __ Branch(&runtime, lt, a3, Operand(zero_reg));  // From < 0.

  __ Branch(&runtime, gt, a3, Operand(a2));  // Fail if from > to.
  __ Dsubu(a2, a2, a3);

  // Make sure first argument is a string.
  __ ld(v0, MemOperand(sp, kStringOffset));
  __ JumpIfSmi(v0, &runtime);
  __ ld(a1, FieldMemOperand(v0, HeapObject::kMapOffset));
  __ lbu(a1, FieldMemOperand(a1, Map::kInstanceTypeOffset));
  __ And(a4, a1, Operand(kIsNotStringMask));

  __ Branch(&runtime, ne, a4, Operand(zero_reg));

  Label single_char;
  __ Branch(&single_char, eq, a2, Operand(1));

  // Short-cut for the case of trivial substring.
  Label return_v0;
  // v0: original string
  // a2: result string length
  __ ld(a4, FieldMemOperand(v0, String::kLengthOffset));
  __ SmiUntag(a4);
  // Return original string.
  __ Branch(&return_v0, eq, a2, Operand(a4));
  // Longer than original string's length or negative: unsafe arguments.
  __ Branch(&runtime, hi, a2, Operand(a4));
  // Shorter than original string's length: an actual substring.

  // Deal with different string types: update the index if necessary
  // and put the underlying string into a5.
  // v0: original string
  // a1: instance type
  // a2: length
  // a3: from index (untagged)
  Label underlying_unpacked, sliced_string, seq_or_external_string;
  // If the string is not indirect, it can only be sequential or external.
  STATIC_ASSERT(kIsIndirectStringMask == (kSlicedStringTag & kConsStringTag));
  STATIC_ASSERT(kIsIndirectStringMask != 0);
  __ And(a4, a1, Operand(kIsIndirectStringMask));
  __ Branch(USE_DELAY_SLOT, &seq_or_external_string, eq, a4, Operand(zero_reg));
  // a4 is used as a scratch register and can be overwritten in either case.
  __ And(a4, a1, Operand(kSlicedNotConsMask));
  __ Branch(&sliced_string, ne, a4, Operand(zero_reg));
  // Cons string.  Check whether it is flat, then fetch first part.
  __ ld(a5, FieldMemOperand(v0, ConsString::kSecondOffset));
  __ LoadRoot(a4, Heap::kempty_stringRootIndex);
  __ Branch(&runtime, ne, a5, Operand(a4));
  __ ld(a5, FieldMemOperand(v0, ConsString::kFirstOffset));
  // Update instance type.
  __ ld(a1, FieldMemOperand(a5, HeapObject::kMapOffset));
  __ lbu(a1, FieldMemOperand(a1, Map::kInstanceTypeOffset));
  __ jmp(&underlying_unpacked);

  __ bind(&sliced_string);
  // Sliced string.  Fetch parent and correct start index by offset.
  __ ld(a5, FieldMemOperand(v0, SlicedString::kParentOffset));
  __ ld(a4, FieldMemOperand(v0, SlicedString::kOffsetOffset));
  __ SmiUntag(a4);  // Add offset to index.
  __ Daddu(a3, a3, a4);
  // Update instance type.
  __ ld(a1, FieldMemOperand(a5, HeapObject::kMapOffset));
  __ lbu(a1, FieldMemOperand(a1, Map::kInstanceTypeOffset));
  __ jmp(&underlying_unpacked);

  __ bind(&seq_or_external_string);
  // Sequential or external string.  Just move string to the expected register.
  __ mov(a5, v0);

  __ bind(&underlying_unpacked);

  if (FLAG_string_slices) {
    Label copy_routine;
    // a5: underlying subject string
    // a1: instance type of underlying subject string
    // a2: length
    // a3: adjusted start index (untagged)
    // Short slice.  Copy instead of slicing.
    __ Branch(&copy_routine, lt, a2, Operand(SlicedString::kMinLength));
    // Allocate new sliced string.  At this point we do not reload the instance
    // type including the string encoding because we simply rely on the info
    // provided by the original string.  It does not matter if the original
    // string's encoding is wrong because we always have to recheck encoding of
    // the newly created string's parent anyways due to externalized strings.
    Label two_byte_slice, set_slice_header;
    STATIC_ASSERT((kStringEncodingMask & kOneByteStringTag) != 0);
    STATIC_ASSERT((kStringEncodingMask & kTwoByteStringTag) == 0);
    __ And(a4, a1, Operand(kStringEncodingMask));
    __ Branch(&two_byte_slice, eq, a4, Operand(zero_reg));
    __ AllocateOneByteSlicedString(v0, a2, a6, a7, &runtime);
    __ jmp(&set_slice_header);
    __ bind(&two_byte_slice);
    __ AllocateTwoByteSlicedString(v0, a2, a6, a7, &runtime);
    __ bind(&set_slice_header);
    __ SmiTag(a3);
    __ sd(a5, FieldMemOperand(v0, SlicedString::kParentOffset));
    __ sd(a3, FieldMemOperand(v0, SlicedString::kOffsetOffset));
    __ jmp(&return_v0);

    __ bind(&copy_routine);
  }

  // a5: underlying subject string
  // a1: instance type of underlying subject string
  // a2: length
  // a3: adjusted start index (untagged)
  Label two_byte_sequential, sequential_string, allocate_result;
  STATIC_ASSERT(kExternalStringTag != 0);
  STATIC_ASSERT(kSeqStringTag == 0);
  __ And(a4, a1, Operand(kExternalStringTag));
  __ Branch(&sequential_string, eq, a4, Operand(zero_reg));

  // Handle external string.
  // Rule out short external strings.
  STATIC_ASSERT(kShortExternalStringTag != 0);
  __ And(a4, a1, Operand(kShortExternalStringTag));
  __ Branch(&runtime, ne, a4, Operand(zero_reg));
  __ ld(a5, FieldMemOperand(a5, ExternalString::kResourceDataOffset));
  // a5 already points to the first character of underlying string.
  __ jmp(&allocate_result);

  __ bind(&sequential_string);
  // Locate first character of underlying subject string.
  STATIC_ASSERT(SeqTwoByteString::kHeaderSize == SeqOneByteString::kHeaderSize);
  __ Daddu(a5, a5, Operand(SeqOneByteString::kHeaderSize - kHeapObjectTag));

  __ bind(&allocate_result);
  // Sequential acii string.  Allocate the result.
  STATIC_ASSERT((kOneByteStringTag & kStringEncodingMask) != 0);
  __ And(a4, a1, Operand(kStringEncodingMask));
  __ Branch(&two_byte_sequential, eq, a4, Operand(zero_reg));

  // Allocate and copy the resulting one_byte string.
  __ AllocateOneByteString(v0, a2, a4, a6, a7, &runtime);

  // Locate first character of substring to copy.
  __ Daddu(a5, a5, a3);

  // Locate first character of result.
  __ Daddu(a1, v0, Operand(SeqOneByteString::kHeaderSize - kHeapObjectTag));

  // v0: result string
  // a1: first character of result string
  // a2: result string length
  // a5: first character of substring to copy
  STATIC_ASSERT((SeqOneByteString::kHeaderSize & kObjectAlignmentMask) == 0);
  StringHelper::GenerateCopyCharacters(
      masm, a1, a5, a2, a3, String::ONE_BYTE_ENCODING);
  __ jmp(&return_v0);

  // Allocate and copy the resulting two-byte string.
  __ bind(&two_byte_sequential);
  __ AllocateTwoByteString(v0, a2, a4, a6, a7, &runtime);

  // Locate first character of substring to copy.
  STATIC_ASSERT(kSmiTagSize == 1 && kSmiTag == 0);
  __ dsll(a4, a3, 1);
  __ Daddu(a5, a5, a4);
  // Locate first character of result.
  __ Daddu(a1, v0, Operand(SeqTwoByteString::kHeaderSize - kHeapObjectTag));

  // v0: result string.
  // a1: first character of result.
  // a2: result length.
  // a5: first character of substring to copy.
  STATIC_ASSERT((SeqTwoByteString::kHeaderSize & kObjectAlignmentMask) == 0);
  StringHelper::GenerateCopyCharacters(
      masm, a1, a5, a2, a3, String::TWO_BYTE_ENCODING);

  __ bind(&return_v0);
  Counters* counters = isolate()->counters();
  __ IncrementCounter(counters->sub_string_native(), 1, a3, a4);
  __ DropAndRet(3);

  // Just jump to runtime to create the sub string.
  __ bind(&runtime);
  __ TailCallRuntime(Runtime::kSubString, 3, 1);

  __ bind(&single_char);
  // v0: original string
  // a1: instance type
  // a2: length
  // a3: from index (untagged)
  StringCharAtGenerator generator(v0, a3, a2, v0, &runtime, &runtime, &runtime,
                                  STRING_INDEX_IS_NUMBER, RECEIVER_IS_STRING);
  generator.GenerateFast(masm);
  __ DropAndRet(3);
  generator.SkipSlow(masm, &runtime);
}


void StringHelper::GenerateFlatOneByteStringEquals(
    MacroAssembler* masm, Register left, Register right, Register scratch1,
    Register scratch2, Register scratch3) {
  Register length = scratch1;

  // Compare lengths.
  Label strings_not_equal, check_zero_length;
  __ ld(length, FieldMemOperand(left, String::kLengthOffset));
  __ ld(scratch2, FieldMemOperand(right, String::kLengthOffset));
  __ Branch(&check_zero_length, eq, length, Operand(scratch2));
  __ bind(&strings_not_equal);
  // Can not put li in delayslot, it has multi instructions.
  __ li(v0, Operand(Smi::FromInt(NOT_EQUAL)));
  __ Ret();

  // Check if the length is zero.
  Label compare_chars;
  __ bind(&check_zero_length);
  STATIC_ASSERT(kSmiTag == 0);
  __ Branch(&compare_chars, ne, length, Operand(zero_reg));
  DCHECK(is_int16((intptr_t)Smi::FromInt(EQUAL)));
  __ Ret(USE_DELAY_SLOT);
  __ li(v0, Operand(Smi::FromInt(EQUAL)));

  // Compare characters.
  __ bind(&compare_chars);

  GenerateOneByteCharsCompareLoop(masm, left, right, length, scratch2, scratch3,
                                  v0, &strings_not_equal);

  // Characters are equal.
  __ Ret(USE_DELAY_SLOT);
  __ li(v0, Operand(Smi::FromInt(EQUAL)));
}


void StringHelper::GenerateCompareFlatOneByteStrings(
    MacroAssembler* masm, Register left, Register right, Register scratch1,
    Register scratch2, Register scratch3, Register scratch4) {
  Label result_not_equal, compare_lengths;
  // Find minimum length and length difference.
  __ ld(scratch1, FieldMemOperand(left, String::kLengthOffset));
  __ ld(scratch2, FieldMemOperand(right, String::kLengthOffset));
  __ Dsubu(scratch3, scratch1, Operand(scratch2));
  Register length_delta = scratch3;
  __ slt(scratch4, scratch2, scratch1);
  __ Movn(scratch1, scratch2, scratch4);
  Register min_length = scratch1;
  STATIC_ASSERT(kSmiTag == 0);
  __ Branch(&compare_lengths, eq, min_length, Operand(zero_reg));

  // Compare loop.
  GenerateOneByteCharsCompareLoop(masm, left, right, min_length, scratch2,
                                  scratch4, v0, &result_not_equal);

  // Compare lengths - strings up to min-length are equal.
  __ bind(&compare_lengths);
  DCHECK(Smi::FromInt(EQUAL) == static_cast<Smi*>(0));
  // Use length_delta as result if it's zero.
  __ mov(scratch2, length_delta);
  __ mov(scratch4, zero_reg);
  __ mov(v0, zero_reg);

  __ bind(&result_not_equal);
  // Conditionally update the result based either on length_delta or
  // the last comparion performed in the loop above.
  Label ret;
  __ Branch(&ret, eq, scratch2, Operand(scratch4));
  __ li(v0, Operand(Smi::FromInt(GREATER)));
  __ Branch(&ret, gt, scratch2, Operand(scratch4));
  __ li(v0, Operand(Smi::FromInt(LESS)));
  __ bind(&ret);
  __ Ret();
}


void StringHelper::GenerateOneByteCharsCompareLoop(
    MacroAssembler* masm, Register left, Register right, Register length,
    Register scratch1, Register scratch2, Register scratch3,
    Label* chars_not_equal) {
  // Change index to run from -length to -1 by adding length to string
  // start. This means that loop ends when index reaches zero, which
  // doesn't need an additional compare.
  __ SmiUntag(length);
  __ Daddu(scratch1, length,
          Operand(SeqOneByteString::kHeaderSize - kHeapObjectTag));
  __ Daddu(left, left, Operand(scratch1));
  __ Daddu(right, right, Operand(scratch1));
  __ Dsubu(length, zero_reg, length);
  Register index = length;  // index = -length;


  // Compare loop.
  Label loop;
  __ bind(&loop);
  __ Daddu(scratch3, left, index);
  __ lbu(scratch1, MemOperand(scratch3));
  __ Daddu(scratch3, right, index);
  __ lbu(scratch2, MemOperand(scratch3));
  __ Branch(chars_not_equal, ne, scratch1, Operand(scratch2));
  __ Daddu(index, index, 1);
  __ Branch(&loop, ne, index, Operand(zero_reg));
}


void StringCompareStub::Generate(MacroAssembler* masm) {
  Label runtime;

  Counters* counters = isolate()->counters();

  // Stack frame on entry.
  //  sp[0]: right string
  //  sp[4]: left string
  __ ld(a1, MemOperand(sp, 1 * kPointerSize));  // Left.
  __ ld(a0, MemOperand(sp, 0 * kPointerSize));  // Right.

  Label not_same;
  __ Branch(&not_same, ne, a0, Operand(a1));
  STATIC_ASSERT(EQUAL == 0);
  STATIC_ASSERT(kSmiTag == 0);
  __ li(v0, Operand(Smi::FromInt(EQUAL)));
  __ IncrementCounter(counters->string_compare_native(), 1, a1, a2);
  __ DropAndRet(2);

  __ bind(&not_same);

  // Check that both objects are sequential one_byte strings.
  __ JumpIfNotBothSequentialOneByteStrings(a1, a0, a2, a3, &runtime);

  // Compare flat one_byte strings natively. Remove arguments from stack first.
  __ IncrementCounter(counters->string_compare_native(), 1, a2, a3);
  __ Daddu(sp, sp, Operand(2 * kPointerSize));
  StringHelper::GenerateCompareFlatOneByteStrings(masm, a1, a0, a2, a3, a4, a5);

  __ bind(&runtime);
  __ TailCallRuntime(Runtime::kStringCompare, 2, 1);
}


void BinaryOpICWithAllocationSiteStub::Generate(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- a1    : left
  //  -- a0    : right
  //  -- ra    : return address
  // -----------------------------------

  // Load a2 with the allocation site. We stick an undefined dummy value here
  // and replace it with the real allocation site later when we instantiate this
  // stub in BinaryOpICWithAllocationSiteStub::GetCodeCopyFromTemplate().
  __ li(a2, handle(isolate()->heap()->undefined_value()));

  // Make sure that we actually patched the allocation site.
  if (FLAG_debug_code) {
    __ And(at, a2, Operand(kSmiTagMask));
    __ Assert(ne, kExpectedAllocationSite, at, Operand(zero_reg));
    __ ld(a4, FieldMemOperand(a2, HeapObject::kMapOffset));
    __ LoadRoot(at, Heap::kAllocationSiteMapRootIndex);
    __ Assert(eq, kExpectedAllocationSite, a4, Operand(at));
  }

  // Tail call into the stub that handles binary operations with allocation
  // sites.
  BinaryOpWithAllocationSiteStub stub(isolate(), state());
  __ TailCallStub(&stub);
}


void CompareICStub::GenerateSmis(MacroAssembler* masm) {
  DCHECK(state() == CompareICState::SMI);
  Label miss;
  __ Or(a2, a1, a0);
  __ JumpIfNotSmi(a2, &miss);

  if (GetCondition() == eq) {
    // For equality we do not care about the sign of the result.
    __ Ret(USE_DELAY_SLOT);
    __ Dsubu(v0, a0, a1);
  } else {
    // Untag before subtracting to avoid handling overflow.
    __ SmiUntag(a1);
    __ SmiUntag(a0);
    __ Ret(USE_DELAY_SLOT);
    __ Dsubu(v0, a1, a0);
  }

  __ bind(&miss);
  GenerateMiss(masm);
}


void CompareICStub::GenerateNumbers(MacroAssembler* masm) {
  DCHECK(state() == CompareICState::NUMBER);

  Label generic_stub;
  Label unordered, maybe_undefined1, maybe_undefined2;
  Label miss;

  if (left() == CompareICState::SMI) {
    __ JumpIfNotSmi(a1, &miss);
  }
  if (right() == CompareICState::SMI) {
    __ JumpIfNotSmi(a0, &miss);
  }

  // Inlining the double comparison and falling back to the general compare
  // stub if NaN is involved.
  // Load left and right operand.
  Label done, left, left_smi, right_smi;
  __ JumpIfSmi(a0, &right_smi);
  __ CheckMap(a0, a2, Heap::kHeapNumberMapRootIndex, &maybe_undefined1,
              DONT_DO_SMI_CHECK);
  __ Dsubu(a2, a0, Operand(kHeapObjectTag));
  __ ldc1(f2, MemOperand(a2, HeapNumber::kValueOffset));
  __ Branch(&left);
  __ bind(&right_smi);
  __ SmiUntag(a2, a0);  // Can't clobber a0 yet.
  FPURegister single_scratch = f6;
  __ mtc1(a2, single_scratch);
  __ cvt_d_w(f2, single_scratch);

  __ bind(&left);
  __ JumpIfSmi(a1, &left_smi);
  __ CheckMap(a1, a2, Heap::kHeapNumberMapRootIndex, &maybe_undefined2,
              DONT_DO_SMI_CHECK);
  __ Dsubu(a2, a1, Operand(kHeapObjectTag));
  __ ldc1(f0, MemOperand(a2, HeapNumber::kValueOffset));
  __ Branch(&done);
  __ bind(&left_smi);
  __ SmiUntag(a2, a1);  // Can't clobber a1 yet.
  single_scratch = f8;
  __ mtc1(a2, single_scratch);
  __ cvt_d_w(f0, single_scratch);

  __ bind(&done);

  // Return a result of -1, 0, or 1, or use CompareStub for NaNs.
  Label fpu_eq, fpu_lt;
  // Test if equal, and also handle the unordered/NaN case.
  __ BranchF(&fpu_eq, &unordered, eq, f0, f2);

  // Test if less (unordered case is already handled).
  __ BranchF(&fpu_lt, NULL, lt, f0, f2);

  // Otherwise it's greater, so just fall thru, and return.
  DCHECK(is_int16(GREATER) && is_int16(EQUAL) && is_int16(LESS));
  __ Ret(USE_DELAY_SLOT);
  __ li(v0, Operand(GREATER));

  __ bind(&fpu_eq);
  __ Ret(USE_DELAY_SLOT);
  __ li(v0, Operand(EQUAL));

  __ bind(&fpu_lt);
  __ Ret(USE_DELAY_SLOT);
  __ li(v0, Operand(LESS));

  __ bind(&unordered);
  __ bind(&generic_stub);
  CompareICStub stub(isolate(), op(), CompareICState::GENERIC,
                     CompareICState::GENERIC, CompareICState::GENERIC);
  __ Jump(stub.GetCode(), RelocInfo::CODE_TARGET);

  __ bind(&maybe_undefined1);
  if (Token::IsOrderedRelationalCompareOp(op())) {
    __ LoadRoot(at, Heap::kUndefinedValueRootIndex);
    __ Branch(&miss, ne, a0, Operand(at));
    __ JumpIfSmi(a1, &unordered);
    __ GetObjectType(a1, a2, a2);
    __ Branch(&maybe_undefined2, ne, a2, Operand(HEAP_NUMBER_TYPE));
    __ jmp(&unordered);
  }

  __ bind(&maybe_undefined2);
  if (Token::IsOrderedRelationalCompareOp(op())) {
    __ LoadRoot(at, Heap::kUndefinedValueRootIndex);
    __ Branch(&unordered, eq, a1, Operand(at));
  }

  __ bind(&miss);
  GenerateMiss(masm);
}


void CompareICStub::GenerateInternalizedStrings(MacroAssembler* masm) {
  DCHECK(state() == CompareICState::INTERNALIZED_STRING);
  Label miss;

  // Registers containing left and right operands respectively.
  Register left = a1;
  Register right = a0;
  Register tmp1 = a2;
  Register tmp2 = a3;

  // Check that both operands are heap objects.
  __ JumpIfEitherSmi(left, right, &miss);

  // Check that both operands are internalized strings.
  __ ld(tmp1, FieldMemOperand(left, HeapObject::kMapOffset));
  __ ld(tmp2, FieldMemOperand(right, HeapObject::kMapOffset));
  __ lbu(tmp1, FieldMemOperand(tmp1, Map::kInstanceTypeOffset));
  __ lbu(tmp2, FieldMemOperand(tmp2, Map::kInstanceTypeOffset));
  STATIC_ASSERT(kInternalizedTag == 0 && kStringTag == 0);
  __ Or(tmp1, tmp1, Operand(tmp2));
  __ And(at, tmp1, Operand(kIsNotStringMask | kIsNotInternalizedMask));
  __ Branch(&miss, ne, at, Operand(zero_reg));

  // Make sure a0 is non-zero. At this point input operands are
  // guaranteed to be non-zero.
  DCHECK(right.is(a0));
  STATIC_ASSERT(EQUAL == 0);
  STATIC_ASSERT(kSmiTag == 0);
  __ mov(v0, right);
  // Internalized strings are compared by identity.
  __ Ret(ne, left, Operand(right));
  DCHECK(is_int16(EQUAL));
  __ Ret(USE_DELAY_SLOT);
  __ li(v0, Operand(Smi::FromInt(EQUAL)));

  __ bind(&miss);
  GenerateMiss(masm);
}


void CompareICStub::GenerateUniqueNames(MacroAssembler* masm) {
  DCHECK(state() == CompareICState::UNIQUE_NAME);
  DCHECK(GetCondition() == eq);
  Label miss;

  // Registers containing left and right operands respectively.
  Register left = a1;
  Register right = a0;
  Register tmp1 = a2;
  Register tmp2 = a3;

  // Check that both operands are heap objects.
  __ JumpIfEitherSmi(left, right, &miss);

  // Check that both operands are unique names. This leaves the instance
  // types loaded in tmp1 and tmp2.
  __ ld(tmp1, FieldMemOperand(left, HeapObject::kMapOffset));
  __ ld(tmp2, FieldMemOperand(right, HeapObject::kMapOffset));
  __ lbu(tmp1, FieldMemOperand(tmp1, Map::kInstanceTypeOffset));
  __ lbu(tmp2, FieldMemOperand(tmp2, Map::kInstanceTypeOffset));

  __ JumpIfNotUniqueNameInstanceType(tmp1, &miss);
  __ JumpIfNotUniqueNameInstanceType(tmp2, &miss);

  // Use a0 as result
  __ mov(v0, a0);

  // Unique names are compared by identity.
  Label done;
  __ Branch(&done, ne, left, Operand(right));
  // Make sure a0 is non-zero. At this point input operands are
  // guaranteed to be non-zero.
  DCHECK(right.is(a0));
  STATIC_ASSERT(EQUAL == 0);
  STATIC_ASSERT(kSmiTag == 0);
  __ li(v0, Operand(Smi::FromInt(EQUAL)));
  __ bind(&done);
  __ Ret();

  __ bind(&miss);
  GenerateMiss(masm);
}


void CompareICStub::GenerateStrings(MacroAssembler* masm) {
  DCHECK(state() == CompareICState::STRING);
  Label miss;

  bool equality = Token::IsEqualityOp(op());

  // Registers containing left and right operands respectively.
  Register left = a1;
  Register right = a0;
  Register tmp1 = a2;
  Register tmp2 = a3;
  Register tmp3 = a4;
  Register tmp4 = a5;
  Register tmp5 = a6;

  // Check that both operands are heap objects.
  __ JumpIfEitherSmi(left, right, &miss);

  // Check that both operands are strings. This leaves the instance
  // types loaded in tmp1 and tmp2.
  __ ld(tmp1, FieldMemOperand(left, HeapObject::kMapOffset));
  __ ld(tmp2, FieldMemOperand(right, HeapObject::kMapOffset));
  __ lbu(tmp1, FieldMemOperand(tmp1, Map::kInstanceTypeOffset));
  __ lbu(tmp2, FieldMemOperand(tmp2, Map::kInstanceTypeOffset));
  STATIC_ASSERT(kNotStringTag != 0);
  __ Or(tmp3, tmp1, tmp2);
  __ And(tmp5, tmp3, Operand(kIsNotStringMask));
  __ Branch(&miss, ne, tmp5, Operand(zero_reg));

  // Fast check for identical strings.
  Label left_ne_right;
  STATIC_ASSERT(EQUAL == 0);
  STATIC_ASSERT(kSmiTag == 0);
  __ Branch(&left_ne_right, ne, left, Operand(right));
  __ Ret(USE_DELAY_SLOT);
  __ mov(v0, zero_reg);  // In the delay slot.
  __ bind(&left_ne_right);

  // Handle not identical strings.

  // Check that both strings are internalized strings. If they are, we're done
  // because we already know they are not identical. We know they are both
  // strings.
  if (equality) {
    DCHECK(GetCondition() == eq);
    STATIC_ASSERT(kInternalizedTag == 0);
    __ Or(tmp3, tmp1, Operand(tmp2));
    __ And(tmp5, tmp3, Operand(kIsNotInternalizedMask));
    Label is_symbol;
    __ Branch(&is_symbol, ne, tmp5, Operand(zero_reg));
    // Make sure a0 is non-zero. At this point input operands are
    // guaranteed to be non-zero.
    DCHECK(right.is(a0));
    __ Ret(USE_DELAY_SLOT);
    __ mov(v0, a0);  // In the delay slot.
    __ bind(&is_symbol);
  }

  // Check that both strings are sequential one_byte.
  Label runtime;
  __ JumpIfBothInstanceTypesAreNotSequentialOneByte(tmp1, tmp2, tmp3, tmp4,
                                                    &runtime);

  // Compare flat one_byte strings. Returns when done.
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
  __ And(a2, a1, Operand(a0));
  __ JumpIfSmi(a2, &miss);

  __ GetObjectType(a0, a2, a2);
  __ Branch(&miss, ne, a2, Operand(JS_OBJECT_TYPE));
  __ GetObjectType(a1, a2, a2);
  __ Branch(&miss, ne, a2, Operand(JS_OBJECT_TYPE));

  DCHECK(GetCondition() == eq);
  __ Ret(USE_DELAY_SLOT);
  __ dsubu(v0, a0, a1);

  __ bind(&miss);
  GenerateMiss(masm);
}


void CompareICStub::GenerateKnownObjects(MacroAssembler* masm) {
  Label miss;
  __ And(a2, a1, a0);
  __ JumpIfSmi(a2, &miss);
  __ ld(a2, FieldMemOperand(a0, HeapObject::kMapOffset));
  __ ld(a3, FieldMemOperand(a1, HeapObject::kMapOffset));
  __ Branch(&miss, ne, a2, Operand(known_map_));
  __ Branch(&miss, ne, a3, Operand(known_map_));

  __ Ret(USE_DELAY_SLOT);
  __ dsubu(v0, a0, a1);

  __ bind(&miss);
  GenerateMiss(masm);
}


void CompareICStub::GenerateMiss(MacroAssembler* masm) {
  {
    // Call the runtime system in a fresh internal frame.
    ExternalReference miss =
        ExternalReference(IC_Utility(IC::kCompareIC_Miss), isolate());
    FrameScope scope(masm, StackFrame::INTERNAL);
    __ Push(a1, a0);
    __ Push(ra, a1, a0);
    __ li(a4, Operand(Smi::FromInt(op())));
    __ daddiu(sp, sp, -kPointerSize);
    __ CallExternalReference(miss, 3, USE_DELAY_SLOT);
    __ sd(a4, MemOperand(sp));  // In the delay slot.
    // Compute the entry point of the rewritten stub.
    __ Daddu(a2, v0, Operand(Code::kHeaderSize - kHeapObjectTag));
    // Restore registers.
    __ Pop(a1, a0, ra);
  }
  __ Jump(a2);
}


void DirectCEntryStub::Generate(MacroAssembler* masm) {
  // Make place for arguments to fit C calling convention. Most of the callers
  // of DirectCEntryStub::GenerateCall are using EnterExitFrame/LeaveExitFrame
  // so they handle stack restoring and we don't have to do that here.
  // Any caller of DirectCEntryStub::GenerateCall must take care of dropping
  // kCArgsSlotsSize stack space after the call.
  __ daddiu(sp, sp, -kCArgsSlotsSize);
  // Place the return address on the stack, making the call
  // GC safe. The RegExp backend also relies on this.
  __ sd(ra, MemOperand(sp, kCArgsSlotsSize));
  __ Call(t9);  // Call the C++ function.
  __ ld(t9, MemOperand(sp, kCArgsSlotsSize));

  if (FLAG_debug_code && FLAG_enable_slow_asserts) {
    // In case of an error the return address may point to a memory area
    // filled with kZapValue by the GC.
    // Dereference the address and check for this.
    __ Uld(a4, MemOperand(t9));
    __ Assert(ne, kReceivedInvalidReturnAddress, a4,
        Operand(reinterpret_cast<uint64_t>(kZapValue)));
  }
  __ Jump(t9);
}


void DirectCEntryStub::GenerateCall(MacroAssembler* masm,
                                    Register target) {
  intptr_t loc =
      reinterpret_cast<intptr_t>(GetCode().location());
  __ Move(t9, target);
  __ li(ra, Operand(loc, RelocInfo::CODE_TARGET), CONSTANT_SIZE);
  __ Call(ra);
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
    __ SmiLoadUntag(index, FieldMemOperand(properties, kCapacityOffset));
    __ Dsubu(index, index, Operand(1));
    __ And(index, index,
           Operand(name->Hash() + NameDictionary::GetProbeOffset(i)));

    // Scale the index by multiplying by the entry size.
    DCHECK(NameDictionary::kEntrySize == 3);
    __ dsll(at, index, 1);
    __ Daddu(index, index, at);  // index *= 3.

    Register entity_name = scratch0;
    // Having undefined at this place means the name is not contained.
    DCHECK_EQ(kSmiTagSize, 1);
    Register tmp = properties;

    __ dsll(scratch0, index, kPointerSizeLog2);
    __ Daddu(tmp, properties, scratch0);
    __ ld(entity_name, FieldMemOperand(tmp, kElementsStartOffset));

    DCHECK(!tmp.is(entity_name));
    __ LoadRoot(tmp, Heap::kUndefinedValueRootIndex);
    __ Branch(done, eq, entity_name, Operand(tmp));

    // Load the hole ready for use below:
    __ LoadRoot(tmp, Heap::kTheHoleValueRootIndex);

    // Stop if found the property.
    __ Branch(miss, eq, entity_name, Operand(Handle<Name>(name)));

    Label good;
    __ Branch(&good, eq, entity_name, Operand(tmp));

    // Check if the entry name is not a unique name.
    __ ld(entity_name, FieldMemOperand(entity_name, HeapObject::kMapOffset));
    __ lbu(entity_name,
           FieldMemOperand(entity_name, Map::kInstanceTypeOffset));
    __ JumpIfNotUniqueNameInstanceType(entity_name, miss);
    __ bind(&good);

    // Restore the properties.
    __ ld(properties,
          FieldMemOperand(receiver, JSObject::kPropertiesOffset));
  }

  const int spill_mask =
      (ra.bit() | a6.bit() | a5.bit() | a4.bit() | a3.bit() |
       a2.bit() | a1.bit() | a0.bit() | v0.bit());

  __ MultiPush(spill_mask);
  __ ld(a0, FieldMemOperand(receiver, JSObject::kPropertiesOffset));
  __ li(a1, Operand(Handle<Name>(name)));
  NameDictionaryLookupStub stub(masm->isolate(), NEGATIVE_LOOKUP);
  __ CallStub(&stub);
  __ mov(at, v0);
  __ MultiPop(spill_mask);

  __ Branch(done, eq, at, Operand(zero_reg));
  __ Branch(miss, ne, at, Operand(zero_reg));
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
  __ ld(scratch1, FieldMemOperand(elements, kCapacityOffset));
  __ SmiUntag(scratch1);
  __ Dsubu(scratch1, scratch1, Operand(1));

  // Generate an unrolled loop that performs a few probes before
  // giving up. Measurements done on Gmail indicate that 2 probes
  // cover ~93% of loads from dictionaries.
  for (int i = 0; i < kInlinedProbes; i++) {
    // Compute the masked index: (hash + i + i * i) & mask.
    __ lwu(scratch2, FieldMemOperand(name, Name::kHashFieldOffset));
    if (i > 0) {
      // Add the probe offset (i + i * i) left shifted to avoid right shifting
      // the hash in a separate instruction. The value hash + i + i * i is right
      // shifted in the following and instruction.
      DCHECK(NameDictionary::GetProbeOffset(i) <
             1 << (32 - Name::kHashFieldOffset));
      __ Daddu(scratch2, scratch2, Operand(
          NameDictionary::GetProbeOffset(i) << Name::kHashShift));
    }
    __ dsrl(scratch2, scratch2, Name::kHashShift);
    __ And(scratch2, scratch1, scratch2);

    // Scale the index by multiplying by the element size.
    DCHECK(NameDictionary::kEntrySize == 3);
    // scratch2 = scratch2 * 3.

    __ dsll(at, scratch2, 1);
    __ Daddu(scratch2, scratch2, at);

    // Check if the key is identical to the name.
    __ dsll(at, scratch2, kPointerSizeLog2);
    __ Daddu(scratch2, elements, at);
    __ ld(at, FieldMemOperand(scratch2, kElementsStartOffset));
    __ Branch(done, eq, name, Operand(at));
  }

  const int spill_mask =
      (ra.bit() | a6.bit() | a5.bit() | a4.bit() |
       a3.bit() | a2.bit() | a1.bit() | a0.bit() | v0.bit()) &
      ~(scratch1.bit() | scratch2.bit());

  __ MultiPush(spill_mask);
  if (name.is(a0)) {
    DCHECK(!elements.is(a1));
    __ Move(a1, name);
    __ Move(a0, elements);
  } else {
    __ Move(a0, elements);
    __ Move(a1, name);
  }
  NameDictionaryLookupStub stub(masm->isolate(), POSITIVE_LOOKUP);
  __ CallStub(&stub);
  __ mov(scratch2, a2);
  __ mov(at, v0);
  __ MultiPop(spill_mask);

  __ Branch(done, ne, at, Operand(zero_reg));
  __ Branch(miss, eq, at, Operand(zero_reg));
}


void NameDictionaryLookupStub::Generate(MacroAssembler* masm) {
  // This stub overrides SometimesSetsUpAFrame() to return false.  That means
  // we cannot call anything that could cause a GC from this stub.
  // Registers:
  //  result: NameDictionary to probe
  //  a1: key
  //  dictionary: NameDictionary to probe.
  //  index: will hold an index of entry if lookup is successful.
  //         might alias with result_.
  // Returns:
  //  result_ is zero if lookup failed, non zero otherwise.

  Register result = v0;
  Register dictionary = a0;
  Register key = a1;
  Register index = a2;
  Register mask = a3;
  Register hash = a4;
  Register undefined = a5;
  Register entry_key = a6;

  Label in_dictionary, maybe_in_dictionary, not_in_dictionary;

  __ ld(mask, FieldMemOperand(dictionary, kCapacityOffset));
  __ SmiUntag(mask);
  __ Dsubu(mask, mask, Operand(1));

  __ lwu(hash, FieldMemOperand(key, Name::kHashFieldOffset));

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
      __ Daddu(index, hash, Operand(
          NameDictionary::GetProbeOffset(i) << Name::kHashShift));
    } else {
      __ mov(index, hash);
    }
    __ dsrl(index, index, Name::kHashShift);
    __ And(index, mask, index);

    // Scale the index by multiplying by the entry size.
    DCHECK(NameDictionary::kEntrySize == 3);
    // index *= 3.
    __ mov(at, index);
    __ dsll(index, index, 1);
    __ Daddu(index, index, at);


    DCHECK_EQ(kSmiTagSize, 1);
    __ dsll(index, index, kPointerSizeLog2);
    __ Daddu(index, index, dictionary);
    __ ld(entry_key, FieldMemOperand(index, kElementsStartOffset));

    // Having undefined at this place means the name is not contained.
    __ Branch(&not_in_dictionary, eq, entry_key, Operand(undefined));

    // Stop if found the property.
    __ Branch(&in_dictionary, eq, entry_key, Operand(key));

    if (i != kTotalProbes - 1 && mode() == NEGATIVE_LOOKUP) {
      // Check if the entry name is not a unique name.
      __ ld(entry_key, FieldMemOperand(entry_key, HeapObject::kMapOffset));
      __ lbu(entry_key,
             FieldMemOperand(entry_key, Map::kInstanceTypeOffset));
      __ JumpIfNotUniqueNameInstanceType(entry_key, &maybe_in_dictionary);
    }
  }

  __ bind(&maybe_in_dictionary);
  // If we are doing negative lookup then probing failure should be
  // treated as a lookup success. For positive lookup probing failure
  // should be treated as lookup failure.
  if (mode() == POSITIVE_LOOKUP) {
    __ Ret(USE_DELAY_SLOT);
    __ mov(result, zero_reg);
  }

  __ bind(&in_dictionary);
  __ Ret(USE_DELAY_SLOT);
  __ li(result, 1);

  __ bind(&not_in_dictionary);
  __ Ret(USE_DELAY_SLOT);
  __ mov(result, zero_reg);
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

  // The first two branch+nop instructions are generated with labels so as to
  // get the offset fixed up correctly by the bind(Label*) call.  We patch it
  // back and forth between a "bne zero_reg, zero_reg, ..." (a nop in this
  // position) and the "beq zero_reg, zero_reg, ..." when we start and stop
  // incremental heap marking.
  // See RecordWriteStub::Patch for details.
  __ beq(zero_reg, zero_reg, &skip_to_incremental_noncompacting);
  __ nop();
  __ beq(zero_reg, zero_reg, &skip_to_incremental_compacting);
  __ nop();

  if (remembered_set_action() == EMIT_REMEMBERED_SET) {
    __ RememberedSetHelper(object(),
                           address(),
                           value(),
                           save_fp_regs_mode(),
                           MacroAssembler::kReturnAtEnd);
  }
  __ Ret();

  __ bind(&skip_to_incremental_noncompacting);
  GenerateIncremental(masm, INCREMENTAL);

  __ bind(&skip_to_incremental_compacting);
  GenerateIncremental(masm, INCREMENTAL_COMPACTION);

  // Initial mode of the stub is expected to be STORE_BUFFER_ONLY.
  // Will be checked in IncrementalMarking::ActivateGeneratedStub.

  PatchBranchIntoNop(masm, 0);
  PatchBranchIntoNop(masm, 2 * Assembler::kInstrSize);
}


void RecordWriteStub::GenerateIncremental(MacroAssembler* masm, Mode mode) {
  regs_.Save(masm);

  if (remembered_set_action() == EMIT_REMEMBERED_SET) {
    Label dont_need_remembered_set;

    __ ld(regs_.scratch0(), MemOperand(regs_.address(), 0));
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
    __ RememberedSetHelper(object(),
                           address(),
                           value(),
                           save_fp_regs_mode(),
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
      a0.is(regs_.address()) ? regs_.scratch0() : regs_.address();
  DCHECK(!address.is(regs_.object()));
  DCHECK(!address.is(a0));
  __ Move(address, regs_.address());
  __ Move(a0, regs_.object());
  __ Move(a1, address);
  __ li(a2, Operand(ExternalReference::isolate_address(isolate())));

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

  __ And(regs_.scratch0(), regs_.object(), Operand(~Page::kPageAlignmentMask));
  __ ld(regs_.scratch1(),
        MemOperand(regs_.scratch0(),
                   MemoryChunk::kWriteBarrierCounterOffset));
  __ Dsubu(regs_.scratch1(), regs_.scratch1(), Operand(1));
  __ sd(regs_.scratch1(),
         MemOperand(regs_.scratch0(),
                    MemoryChunk::kWriteBarrierCounterOffset));
  __ Branch(&need_incremental, lt, regs_.scratch1(), Operand(zero_reg));

  // Let's look at the color of the object:  If it is not black we don't have
  // to inform the incremental marker.
  __ JumpIfBlack(regs_.object(), regs_.scratch0(), regs_.scratch1(), &on_black);

  regs_.Restore(masm);
  if (on_no_need == kUpdateRememberedSetOnNoNeedToInformIncrementalMarker) {
    __ RememberedSetHelper(object(),
                           address(),
                           value(),
                           save_fp_regs_mode(),
                           MacroAssembler::kReturnAtEnd);
  } else {
    __ Ret();
  }

  __ bind(&on_black);

  // Get the value from the slot.
  __ ld(regs_.scratch0(), MemOperand(regs_.address(), 0));

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
    __ RememberedSetHelper(object(),
                           address(),
                           value(),
                           save_fp_regs_mode(),
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
  //  -- a0    : element value to store
  //  -- a3    : element index as smi
  //  -- sp[0] : array literal index in function as smi
  //  -- sp[4] : array literal
  // clobbers a1, a2, a4
  // -----------------------------------

  Label element_done;
  Label double_elements;
  Label smi_element;
  Label slow_elements;
  Label fast_elements;

  // Get array literal index, array literal and its map.
  __ ld(a4, MemOperand(sp, 0 * kPointerSize));
  __ ld(a1, MemOperand(sp, 1 * kPointerSize));
  __ ld(a2, FieldMemOperand(a1, JSObject::kMapOffset));

  __ CheckFastElements(a2, a5, &double_elements);
  // Check for FAST_*_SMI_ELEMENTS or FAST_*_ELEMENTS elements
  __ JumpIfSmi(a0, &smi_element);
  __ CheckFastSmiElements(a2, a5, &fast_elements);

  // Store into the array literal requires a elements transition. Call into
  // the runtime.
  __ bind(&slow_elements);
  // call.
  __ Push(a1, a3, a0);
  __ ld(a5, MemOperand(fp, JavaScriptFrameConstants::kFunctionOffset));
  __ ld(a5, FieldMemOperand(a5, JSFunction::kLiteralsOffset));
  __ Push(a5, a4);
  __ TailCallRuntime(Runtime::kStoreArrayLiteralElement, 5, 1);

  // Array literal has ElementsKind of FAST_*_ELEMENTS and value is an object.
  __ bind(&fast_elements);
  __ ld(a5, FieldMemOperand(a1, JSObject::kElementsOffset));
  __ SmiScale(a6, a3, kPointerSizeLog2);
  __ Daddu(a6, a5, a6);
  __ Daddu(a6, a6, Operand(FixedArray::kHeaderSize - kHeapObjectTag));
  __ sd(a0, MemOperand(a6, 0));
  // Update the write barrier for the array store.
  __ RecordWrite(a5, a6, a0, kRAHasNotBeenSaved, kDontSaveFPRegs,
                 EMIT_REMEMBERED_SET, OMIT_SMI_CHECK);
  __ Ret(USE_DELAY_SLOT);
  __ mov(v0, a0);

  // Array literal has ElementsKind of FAST_*_SMI_ELEMENTS or FAST_*_ELEMENTS,
  // and value is Smi.
  __ bind(&smi_element);
  __ ld(a5, FieldMemOperand(a1, JSObject::kElementsOffset));
  __ SmiScale(a6, a3, kPointerSizeLog2);
  __ Daddu(a6, a5, a6);
  __ sd(a0, FieldMemOperand(a6, FixedArray::kHeaderSize));
  __ Ret(USE_DELAY_SLOT);
  __ mov(v0, a0);

  // Array literal has ElementsKind of FAST_*_DOUBLE_ELEMENTS.
  __ bind(&double_elements);
  __ ld(a5, FieldMemOperand(a1, JSObject::kElementsOffset));
  __ StoreNumberToDoubleElements(a0, a3, a5, a7, t1, a2, &slow_elements);
  __ Ret(USE_DELAY_SLOT);
  __ mov(v0, a0);
}


void StubFailureTrampolineStub::Generate(MacroAssembler* masm) {
  CEntryStub ces(isolate(), 1, kSaveFPRegs);
  __ Call(ces.GetCode(), RelocInfo::CODE_TARGET);
  int parameter_count_offset =
      StubFailureTrampolineFrame::kCallerStackParameterCountFrameOffset;
  __ ld(a1, MemOperand(fp, parameter_count_offset));
  if (function_mode() == JS_FUNCTION_STUB_MODE) {
    __ Daddu(a1, a1, Operand(1));
  }
  masm->LeaveFrame(StackFrame::STUB_FAILURE_TRAMPOLINE);
  __ dsll(a1, a1, kPointerSizeLog2);
  __ Ret(USE_DELAY_SLOT);
  __ Daddu(sp, sp, a1);
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
    __ push(ra);
    __ CallStub(&stub);
    __ pop(ra);
  }
}


void ProfileEntryHookStub::Generate(MacroAssembler* masm) {
  // The entry hook is a "push ra" instruction, followed by a call.
  // Note: on MIPS "push" is 2 instruction
  const int32_t kReturnAddressDistanceFromFunctionStart =
      Assembler::kCallTargetAddressOffset + (2 * Assembler::kInstrSize);

  // This should contain all kJSCallerSaved registers.
  const RegList kSavedRegs =
     kJSCallerSaved |  // Caller saved registers.
     s5.bit();         // Saved stack pointer.

  // We also save ra, so the count here is one higher than the mask indicates.
  const int32_t kNumSavedRegs = kNumJSCallerSaved + 2;

  // Save all caller-save registers as this may be called from anywhere.
  __ MultiPush(kSavedRegs | ra.bit());

  // Compute the function's address for the first argument.
  __ Dsubu(a0, ra, Operand(kReturnAddressDistanceFromFunctionStart));

  // The caller's return address is above the saved temporaries.
  // Grab that for the second argument to the hook.
  __ Daddu(a1, sp, Operand(kNumSavedRegs * kPointerSize));

  // Align the stack if necessary.
  int frame_alignment = masm->ActivationFrameAlignment();
  if (frame_alignment > kPointerSize) {
    __ mov(s5, sp);
    DCHECK(base::bits::IsPowerOfTwo32(frame_alignment));
    __ And(sp, sp, Operand(-frame_alignment));
  }

  __ Dsubu(sp, sp, kCArgsSlotsSize);
#if defined(V8_HOST_ARCH_MIPS) || defined(V8_HOST_ARCH_MIPS64)
  int64_t entry_hook =
      reinterpret_cast<int64_t>(isolate()->function_entry_hook());
  __ li(t9, Operand(entry_hook));
#else
  // Under the simulator we need to indirect the entry hook through a
  // trampoline function at a known address.
  // It additionally takes an isolate as a third parameter.
  __ li(a2, Operand(ExternalReference::isolate_address(isolate())));

  ApiFunction dispatcher(FUNCTION_ADDR(EntryHookTrampoline));
  __ li(t9, Operand(ExternalReference(&dispatcher,
                                      ExternalReference::BUILTIN_CALL,
                                      isolate())));
#endif
  // Call C function through t9 to conform ABI for PIC.
  __ Call(t9);

  // Restore the stack pointer if needed.
  if (frame_alignment > kPointerSize) {
    __ mov(sp, s5);
  } else {
    __ Daddu(sp, sp, kCArgsSlotsSize);
  }

  // Also pop ra to get Ret(0).
  __ MultiPop(kSavedRegs | ra.bit());
  __ Ret();
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
      T stub(masm->isolate(), kind);
      __ TailCallStub(&stub, eq, a3, Operand(kind));
    }

    // If we reached this point there is a problem.
    __ Abort(kUnexpectedElementsKindInArrayConstructor);
  } else {
    UNREACHABLE();
  }
}


static void CreateArrayDispatchOneArgument(MacroAssembler* masm,
                                           AllocationSiteOverrideMode mode) {
  // a2 - allocation site (if mode != DISABLE_ALLOCATION_SITES)
  // a3 - kind (if mode != DISABLE_ALLOCATION_SITES)
  // a0 - number of arguments
  // a1 - constructor?
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
    __ And(at, a3, Operand(1));
    __ Branch(&normal_sequence, ne, at, Operand(zero_reg));
  }
  // look at the first argument
  __ ld(a5, MemOperand(sp, 0));
  __ Branch(&normal_sequence, eq, a5, Operand(zero_reg));

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
    __ Daddu(a3, a3, Operand(1));

    if (FLAG_debug_code) {
      __ ld(a5, FieldMemOperand(a2, 0));
      __ LoadRoot(at, Heap::kAllocationSiteMapRootIndex);
      __ Assert(eq, kExpectedAllocationSite, a5, Operand(at));
    }

    // Save the resulting elements kind in type info. We can't just store a3
    // in the AllocationSite::transition_info field because elements kind is
    // restricted to a portion of the field...upper bits need to be left alone.
    STATIC_ASSERT(AllocationSite::ElementsKindBits::kShift == 0);
    __ ld(a4, FieldMemOperand(a2, AllocationSite::kTransitionInfoOffset));
    __ Daddu(a4, a4, Operand(Smi::FromInt(kFastElementsKindPackedToHoley)));
    __ sd(a4, FieldMemOperand(a2, AllocationSite::kTransitionInfoOffset));


    __ bind(&normal_sequence);
    int last_index = GetSequenceIndexFromFastElementsKind(
        TERMINAL_FAST_ELEMENTS_KIND);
    for (int i = 0; i <= last_index; ++i) {
      ElementsKind kind = GetFastElementsKindFromSequenceIndex(i);
      ArraySingleArgumentConstructorStub stub(masm->isolate(), kind);
      __ TailCallStub(&stub, eq, a3, Operand(kind));
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
    // For internal arrays we only need a few things.
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
    __ And(at, a0, a0);
    __ Branch(&not_zero_case, ne, at, Operand(zero_reg));
    CreateArrayDispatch<ArrayNoArgumentConstructorStub>(masm, mode);

    __ bind(&not_zero_case);
    __ Branch(&not_one_case, gt, a0, Operand(1));
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
  //  -- a0 : argc (only if argument_count() == ANY)
  //  -- a1 : constructor
  //  -- a2 : AllocationSite or undefined
  //  -- sp[0] : return address
  //  -- sp[4] : last argument
  // -----------------------------------

  if (FLAG_debug_code) {
    // The array construct code is only set for the global and natives
    // builtin Array functions which always have maps.

    // Initial map for the builtin Array function should be a map.
    __ ld(a4, FieldMemOperand(a1, JSFunction::kPrototypeOrInitialMapOffset));
    // Will both indicate a NULL and a Smi.
    __ SmiTst(a4, at);
    __ Assert(ne, kUnexpectedInitialMapForArrayFunction,
        at, Operand(zero_reg));
    __ GetObjectType(a4, a4, a5);
    __ Assert(eq, kUnexpectedInitialMapForArrayFunction,
        a5, Operand(MAP_TYPE));

    // We should either have undefined in a2 or a valid AllocationSite
    __ AssertUndefinedOrAllocationSite(a2, a4);
  }

  Label no_info;
  // Get the elements kind and case on that.
  __ LoadRoot(at, Heap::kUndefinedValueRootIndex);
  __ Branch(&no_info, eq, a2, Operand(at));

  __ ld(a3, FieldMemOperand(a2, AllocationSite::kTransitionInfoOffset));
  __ SmiUntag(a3);
  STATIC_ASSERT(AllocationSite::ElementsKindBits::kShift == 0);
  __ And(a3, a3, Operand(AllocationSite::ElementsKindBits::kMask));
  GenerateDispatchToArrayStub(masm, DONT_OVERRIDE);

  __ bind(&no_info);
  GenerateDispatchToArrayStub(masm, DISABLE_ALLOCATION_SITES);
}


void InternalArrayConstructorStub::GenerateCase(
    MacroAssembler* masm, ElementsKind kind) {

  InternalArrayNoArgumentConstructorStub stub0(isolate(), kind);
  __ TailCallStub(&stub0, lo, a0, Operand(1));

  InternalArrayNArgumentsConstructorStub stubN(isolate(), kind);
  __ TailCallStub(&stubN, hi, a0, Operand(1));

  if (IsFastPackedElementsKind(kind)) {
    // We might need to create a holey array
    // look at the first argument.
    __ ld(at, MemOperand(sp, 0));

    InternalArraySingleArgumentConstructorStub
        stub1_holey(isolate(), GetHoleyElementsKind(kind));
    __ TailCallStub(&stub1_holey, ne, at, Operand(zero_reg));
  }

  InternalArraySingleArgumentConstructorStub stub1(isolate(), kind);
  __ TailCallStub(&stub1);
}


void InternalArrayConstructorStub::Generate(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- a0 : argc
  //  -- a1 : constructor
  //  -- sp[0] : return address
  //  -- sp[4] : last argument
  // -----------------------------------

  if (FLAG_debug_code) {
    // The array construct code is only set for the global and natives
    // builtin Array functions which always have maps.

    // Initial map for the builtin Array function should be a map.
    __ ld(a3, FieldMemOperand(a1, JSFunction::kPrototypeOrInitialMapOffset));
    // Will both indicate a NULL and a Smi.
    __ SmiTst(a3, at);
    __ Assert(ne, kUnexpectedInitialMapForArrayFunction,
        at, Operand(zero_reg));
    __ GetObjectType(a3, a3, a4);
    __ Assert(eq, kUnexpectedInitialMapForArrayFunction,
        a4, Operand(MAP_TYPE));
  }

  // Figure out the right elements kind.
  __ ld(a3, FieldMemOperand(a1, JSFunction::kPrototypeOrInitialMapOffset));

  // Load the map's "bit field 2" into a3. We only need the first byte,
  // but the following bit field extraction takes care of that anyway.
  __ lbu(a3, FieldMemOperand(a3, Map::kBitField2Offset));
  // Retrieve elements_kind from bit field 2.
  __ DecodeField<Map::ElementsKindBits>(a3);

  if (FLAG_debug_code) {
    Label done;
    __ Branch(&done, eq, a3, Operand(FAST_ELEMENTS));
    __ Assert(
        eq, kInvalidElementsKindForInternalArrayOrInternalPackedArray,
        a3, Operand(FAST_HOLEY_ELEMENTS));
    __ bind(&done);
  }

  Label fast_elements_case;
  __ Branch(&fast_elements_case, eq, a3, Operand(FAST_ELEMENTS));
  GenerateCase(masm, FAST_HOLEY_ELEMENTS);

  __ bind(&fast_elements_case);
  GenerateCase(masm, FAST_ELEMENTS);
}


void CallApiFunctionStub::Generate(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- a0                  : callee
  //  -- a4                  : call_data
  //  -- a2                  : holder
  //  -- a1                  : api_function_address
  //  -- cp                  : context
  //  --
  //  -- sp[0]               : last argument
  //  -- ...
  //  -- sp[(argc - 1)* 4]   : first argument
  //  -- sp[argc * 4]        : receiver
  // -----------------------------------

  Register callee = a0;
  Register call_data = a4;
  Register holder = a2;
  Register api_function_address = a1;
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

  // Save context, callee and call data.
  __ Push(context, callee, call_data);
  // Load context from callee.
  __ ld(context, FieldMemOperand(callee, JSFunction::kContextOffset));

  Register scratch = call_data;
  if (!call_data_undefined) {
    __ LoadRoot(scratch, Heap::kUndefinedValueRootIndex);
  }
  // Push return value and default return value.
  __ Push(scratch, scratch);
  __ li(scratch,
        Operand(ExternalReference::isolate_address(isolate())));
  // Push isolate and holder.
  __ Push(scratch, holder);

  // Prepare arguments.
  __ mov(scratch, sp);

  // Allocate the v8::Arguments structure in the arguments' space since
  // it's not controlled by GC.
  const int kApiStackSpace = 4;

  FrameScope frame_scope(masm, StackFrame::MANUAL);
  __ EnterExitFrame(false, kApiStackSpace);

  DCHECK(!api_function_address.is(a0) && !scratch.is(a0));
  // a0 = FunctionCallbackInfo&
  // Arguments is after the return address.
  __ Daddu(a0, sp, Operand(1 * kPointerSize));
  // FunctionCallbackInfo::implicit_args_
  __ sd(scratch, MemOperand(a0, 0 * kPointerSize));
  // FunctionCallbackInfo::values_
  __ Daddu(at, scratch, Operand((FCA::kArgsLength - 1 + argc) * kPointerSize));
  __ sd(at, MemOperand(a0, 1 * kPointerSize));
  // FunctionCallbackInfo::length_ = argc
  __ li(at, Operand(argc));
  __ sd(at, MemOperand(a0, 2 * kPointerSize));
  // FunctionCallbackInfo::is_construct_call = 0
  __ sd(zero_reg, MemOperand(a0, 3 * kPointerSize));

  const int kStackUnwindSpace = argc + FCA::kArgsLength + 1;
  ExternalReference thunk_ref =
      ExternalReference::invoke_function_callback(isolate());

  AllowExternalCallThatCantCauseGC scope(masm);
  MemOperand context_restore_operand(
      fp, (2 + FCA::kContextSaveIndex) * kPointerSize);
  // Stores return the first js argument.
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
  //  -- a2                     : api_function_address
  // -----------------------------------

  Register api_function_address = ApiGetterDescriptor::function_address();
  DCHECK(api_function_address.is(a2));

  __ mov(a0, sp);  // a0 = Handle<Name>
  __ Daddu(a1, a0, Operand(1 * kPointerSize));  // a1 = PCA

  const int kApiStackSpace = 1;
  FrameScope frame_scope(masm, StackFrame::MANUAL);
  __ EnterExitFrame(false, kApiStackSpace);

  // Create PropertyAccessorInfo instance on the stack above the exit frame with
  // a1 (internal::Object** args_) as the data.
  __ sd(a1, MemOperand(sp, 1 * kPointerSize));
  __ Daddu(a1, sp, Operand(1 * kPointerSize));  // a1 = AccessorInfo&

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

#endif  // V8_TARGET_ARCH_MIPS64
