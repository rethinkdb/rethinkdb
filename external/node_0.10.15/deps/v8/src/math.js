// Copyright 2012 the V8 project authors. All rights reserved.
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


// Keep reference to original values of some global properties.  This
// has the added benefit that the code in this file is isolated from
// changes to these properties.
var $floor = MathFloor;
var $abs = MathAbs;

// Instance class name can only be set on functions. That is the only
// purpose for MathConstructor.
function MathConstructor() {}
%FunctionSetInstanceClassName(MathConstructor, 'Math');
var $Math = new MathConstructor();
$Math.__proto__ = $Object.prototype;
%SetProperty(global, "Math", $Math, DONT_ENUM);

// ECMA 262 - 15.8.2.1
function MathAbs(x) {
  if (%_IsSmi(x)) return x >= 0 ? x : -x;
  if (!IS_NUMBER(x)) x = NonNumberToNumber(x);
  if (x === 0) return 0;  // To handle -0.
  return x > 0 ? x : -x;
}

// ECMA 262 - 15.8.2.2
function MathAcos(x) {
  if (!IS_NUMBER(x)) x = NonNumberToNumber(x);
  return %Math_acos(x);
}

// ECMA 262 - 15.8.2.3
function MathAsin(x) {
  if (!IS_NUMBER(x)) x = NonNumberToNumber(x);
  return %Math_asin(x);
}

// ECMA 262 - 15.8.2.4
function MathAtan(x) {
  if (!IS_NUMBER(x)) x = NonNumberToNumber(x);
  return %Math_atan(x);
}

// ECMA 262 - 15.8.2.5
// The naming of y and x matches the spec, as does the order in which
// ToNumber (valueOf) is called.
function MathAtan2(y, x) {
  if (!IS_NUMBER(y)) y = NonNumberToNumber(y);
  if (!IS_NUMBER(x)) x = NonNumberToNumber(x);
  return %Math_atan2(y, x);
}

// ECMA 262 - 15.8.2.6
function MathCeil(x) {
  if (!IS_NUMBER(x)) x = NonNumberToNumber(x);
  return %Math_ceil(x);
}

// ECMA 262 - 15.8.2.7
function MathCos(x) {
  if (!IS_NUMBER(x)) x = NonNumberToNumber(x);
  return %_MathCos(x);
}

// ECMA 262 - 15.8.2.8
function MathExp(x) {
  if (!IS_NUMBER(x)) x = NonNumberToNumber(x);
  return %Math_exp(x);
}

// ECMA 262 - 15.8.2.9
function MathFloor(x) {
  if (!IS_NUMBER(x)) x = NonNumberToNumber(x);
  // It's more common to call this with a positive number that's out
  // of range than negative numbers; check the upper bound first.
  if (x < 0x80000000 && x > 0) {
    // Numbers in the range [0, 2^31) can be floored by converting
    // them to an unsigned 32-bit value using the shift operator.
    // We avoid doing so for -0, because the result of Math.floor(-0)
    // has to be -0, which wouldn't be the case with the shift.
    return TO_UINT32(x);
  } else {
    return %Math_floor(x);
  }
}

// ECMA 262 - 15.8.2.10
function MathLog(x) {
  if (!IS_NUMBER(x)) x = NonNumberToNumber(x);
  return %_MathLog(x);
}

// ECMA 262 - 15.8.2.11
function MathMax(arg1, arg2) {  // length == 2
  var length = %_ArgumentsLength();
  if (length == 2) {
    if (!IS_NUMBER(arg1)) arg1 = NonNumberToNumber(arg1);
    if (!IS_NUMBER(arg2)) arg2 = NonNumberToNumber(arg2);
    if (arg2 > arg1) return arg2;
    if (arg1 > arg2) return arg1;
    if (arg1 == arg2) {
      // Make sure -0 is considered less than +0.  -0 is never a Smi, +0 can be
      // a Smi or a heap number.
      return (arg1 == 0 && !%_IsSmi(arg1) && 1 / arg1 < 0) ? arg2 : arg1;
    }
    // All comparisons failed, one of the arguments must be NaN.
    return 0/0;  // Compiler constant-folds this to NaN.
  }
  if (length == 0) {
    return -1/0;  // Compiler constant-folds this to -Infinity.
  }
  var r = arg1;
  if (!IS_NUMBER(r)) r = NonNumberToNumber(r);
  if (NUMBER_IS_NAN(r)) return r;
  for (var i = 1; i < length; i++) {
    var n = %_Arguments(i);
    if (!IS_NUMBER(n)) n = NonNumberToNumber(n);
    if (NUMBER_IS_NAN(n)) return n;
    // Make sure +0 is considered greater than -0.  -0 is never a Smi, +0 can be
    // a Smi or heap number.
    if (n > r || (r == 0 && n == 0 && !%_IsSmi(r) && 1 / r < 0)) r = n;
  }
  return r;
}

// ECMA 262 - 15.8.2.12
function MathMin(arg1, arg2) {  // length == 2
  var length = %_ArgumentsLength();
  if (length == 2) {
    if (!IS_NUMBER(arg1)) arg1 = NonNumberToNumber(arg1);
    if (!IS_NUMBER(arg2)) arg2 = NonNumberToNumber(arg2);
    if (arg2 > arg1) return arg1;
    if (arg1 > arg2) return arg2;
    if (arg1 == arg2) {
      // Make sure -0 is considered less than +0.  -0 is never a Smi, +0 can be
      // a Smi or a heap number.
      return (arg1 == 0 && !%_IsSmi(arg1) && 1 / arg1 < 0) ? arg1 : arg2;
    }
    // All comparisons failed, one of the arguments must be NaN.
    return 0/0;  // Compiler constant-folds this to NaN.
  }
  if (length == 0) {
    return 1/0;  // Compiler constant-folds this to Infinity.
  }
  var r = arg1;
  if (!IS_NUMBER(r)) r = NonNumberToNumber(r);
  if (NUMBER_IS_NAN(r)) return r;
  for (var i = 1; i < length; i++) {
    var n = %_Arguments(i);
    if (!IS_NUMBER(n)) n = NonNumberToNumber(n);
    if (NUMBER_IS_NAN(n)) return n;
    // Make sure -0 is considered less than +0.  -0 is never a Smi, +0 can be a
    // Smi or a heap number.
    if (n < r || (r == 0 && n == 0 && !%_IsSmi(n) && 1 / n < 0)) r = n;
  }
  return r;
}

// ECMA 262 - 15.8.2.13
function MathPow(x, y) {
  if (!IS_NUMBER(x)) x = NonNumberToNumber(x);
  if (!IS_NUMBER(y)) y = NonNumberToNumber(y);
  return %_MathPow(x, y);
}

// ECMA 262 - 15.8.2.14
function MathRandom() {
  return %_RandomHeapNumber();
}

// ECMA 262 - 15.8.2.15
function MathRound(x) {
  if (!IS_NUMBER(x)) x = NonNumberToNumber(x);
  return %RoundNumber(x);
}

// ECMA 262 - 15.8.2.16
function MathSin(x) {
  if (!IS_NUMBER(x)) x = NonNumberToNumber(x);
  return %_MathSin(x);
}

// ECMA 262 - 15.8.2.17
function MathSqrt(x) {
  if (!IS_NUMBER(x)) x = NonNumberToNumber(x);
  return %_MathSqrt(x);
}

// ECMA 262 - 15.8.2.18
function MathTan(x) {
  if (!IS_NUMBER(x)) x = NonNumberToNumber(x);
  return %_MathTan(x);
}


// -------------------------------------------------------------------

function SetUpMath() {
  %CheckIsBootstrapping();
  // Set up math constants.
  // ECMA-262, section 15.8.1.1.
  %OptimizeObjectForAddingMultipleProperties($Math, 8);
  %SetProperty($Math,
               "E",
               2.7182818284590452354,
               DONT_ENUM |  DONT_DELETE | READ_ONLY);
  // ECMA-262, section 15.8.1.2.
  %SetProperty($Math,
               "LN10",
               2.302585092994046,
               DONT_ENUM |  DONT_DELETE | READ_ONLY);
  // ECMA-262, section 15.8.1.3.
  %SetProperty($Math,
               "LN2",
               0.6931471805599453,
               DONT_ENUM |  DONT_DELETE | READ_ONLY);
  // ECMA-262, section 15.8.1.4.
  %SetProperty($Math,
               "LOG2E",
               1.4426950408889634,
               DONT_ENUM |  DONT_DELETE | READ_ONLY);
  %SetProperty($Math,
               "LOG10E",
               0.4342944819032518,
               DONT_ENUM |  DONT_DELETE | READ_ONLY);
  %SetProperty($Math,
               "PI",
               3.1415926535897932,
               DONT_ENUM |  DONT_DELETE | READ_ONLY);
  %SetProperty($Math,
               "SQRT1_2",
               0.7071067811865476,
               DONT_ENUM |  DONT_DELETE | READ_ONLY);
  %SetProperty($Math,
               "SQRT2",
               1.4142135623730951,
               DONT_ENUM |  DONT_DELETE | READ_ONLY);
  %ToFastProperties($Math);

  // Set up non-enumerable functions of the Math object and
  // set their names.
  InstallFunctions($Math, DONT_ENUM, $Array(
    "random", MathRandom,
    "abs", MathAbs,
    "acos", MathAcos,
    "asin", MathAsin,
    "atan", MathAtan,
    "ceil", MathCeil,
    "cos", MathCos,
    "exp", MathExp,
    "floor", MathFloor,
    "log", MathLog,
    "round", MathRound,
    "sin", MathSin,
    "sqrt", MathSqrt,
    "tan", MathTan,
    "atan2", MathAtan2,
    "pow", MathPow,
    "max", MathMax,
    "min", MathMin
  ));
}

SetUpMath();
