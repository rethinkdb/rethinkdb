// Copyright (C) 2011 Milo Yip
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef RAPIDJSON_STRTOD_
#define RAPIDJSON_STRTOD_

#include "../rapidjson.h"
#include "ieee754.h"
#include "biginteger.h"
#include "diyfp.h"
#include "pow10.h"

RAPIDJSON_NAMESPACE_BEGIN
namespace internal {

inline double FastPath(double significand, int exp) {
    if (exp < -308)
        return 0.0;
    else if (exp >= 0)
        return significand * internal::Pow10(exp);
    else
        return significand / internal::Pow10(-exp);
}

inline double StrtodNormalPrecision(double d, int p) {
    if (p < -308) {
        // Prevent expSum < -308, making Pow10(p) = 0
        d = FastPath(d, -308);
        d = FastPath(d, p + 308);
    }
    else
        d = FastPath(d, p);
    return d;
}

template <typename T>
inline T Min3(T a, T b, T c) {
    T m = a;
    if (m > b) m = b;
    if (m > c) m = c;
    return m;
}

inline int CheckWithinHalfULP(double b, const BigInteger& d, int dExp, bool* adjustToNegative) {
    const Double db(b);
    const uint64_t bInt = db.IntegerSignificand();
    const int bExp = db.IntegerExponent();
    const int hExp = bExp - 1;

    int dS_Exp2 = 0, dS_Exp5 = 0, bS_Exp2 = 0, bS_Exp5 = 0, hS_Exp2 = 0, hS_Exp5 = 0;

    // Adjust for decimal exponent
    if (dExp >= 0) {
        dS_Exp2 += dExp;
        dS_Exp5 += dExp;
    }
    else {
        bS_Exp2 -= dExp;
        bS_Exp5 -= dExp;
        hS_Exp2 -= dExp;
        hS_Exp5 -= dExp;
    }

    // Adjust for binary exponent
    if (bExp >= 0)
        bS_Exp2 += bExp;
    else {
        dS_Exp2 -= bExp;
        hS_Exp2 -= bExp;
    }

    // Adjust for half ulp exponent
    if (hExp >= 0)
        hS_Exp2 += hExp;
    else {
        dS_Exp2 -= hExp;
        bS_Exp2 -= hExp;
    }

    // Remove common power of two factor from all three scaled values
    int common_Exp2 = Min3(dS_Exp2, bS_Exp2, hS_Exp2);
    dS_Exp2 -= common_Exp2;
    bS_Exp2 -= common_Exp2;
    hS_Exp2 -= common_Exp2;

    BigInteger dS = d;
    dS.MultiplyPow5(dS_Exp5) <<= dS_Exp2;

    BigInteger bS(bInt);
    bS.MultiplyPow5(bS_Exp5) <<= bS_Exp2;

    BigInteger hS(1);
    hS.MultiplyPow5(hS_Exp5) <<= hS_Exp2;

    BigInteger delta(0);
    *adjustToNegative = dS.Difference(bS, &delta);

    int cmp = delta.Compare(hS);
    // If delta is within 1/2 ULP, check for special case when significand is power of two.
    // In this case, need to compare with 1/2h in the lower bound.
    if (cmp < 0 && *adjustToNegative && // within and dS < bS
        db.IsNormal() && (bInt & (bInt - 1)) == 0 && // Power of 2
        db.Uint64Value() != RAPIDJSON_UINT64_C2(0x00100000, 0x00000000)) // minimum normal number must not do this
    {
        delta <<= 1;
        return delta.Compare(hS);
    }
    return cmp;
}

inline bool StrtodFast(double d, int p, double* result) {
    // Use fast path for string-to-double conversion if possible
    // see http://www.exploringbinary.com/fast-path-decimal-to-floating-point-conversion/
    if (p > 22  && p < 22 + 16) {
        // Fast Path Cases In Disguise
        d *= internal::Pow10(p - 22);
        p = 22;
    }

    if (p >= -22 && p <= 22 && d <= 9007199254740991.0) { // 2^53 - 1
        *result = FastPath(d, p);
        return true;
    }
    else
        return false;
}

// Compute an approximation and see if it is within 1/2 ULP
inline bool StrtodDiyFp(const char* decimals, size_t length, size_t decimalPosition, int exp, double* result) {
    uint64_t significand = 0;
    size_t i = 0;   // 2^64 - 1 = 18446744073709551615, 1844674407370955161 = 0x1999999999999999    
    for (; i < length; i++) {
        if (significand  >  RAPIDJSON_UINT64_C2(0x19999999, 0x99999999) ||
            (significand == RAPIDJSON_UINT64_C2(0x19999999, 0x99999999) && decimals[i] > '5'))
            break;
        significand = significand * 10 + (decimals[i] - '0');
    }
    
    if (i < length && decimals[i] >= '5') // Rounding
        significand++;

    size_t remaining = length - i;
    const unsigned kUlpShift = 3;
    const unsigned kUlp = 1 << kUlpShift;
    int error = (remaining == 0) ? 0 : kUlp / 2;

    DiyFp v(significand, 0);
    v = v.Normalize();
    error <<= -v.e;

    const int dExp = (int)decimalPosition - (int)i + exp;

    int actualExp;
    DiyFp cachedPower = GetCachedPower10(dExp, &actualExp);
    if (actualExp != dExp) {
        static const DiyFp kPow10[] = {
            DiyFp(RAPIDJSON_UINT64_C2(0xa0000000, 00000000), -60),  // 10^1
            DiyFp(RAPIDJSON_UINT64_C2(0xc8000000, 00000000), -57),  // 10^2
            DiyFp(RAPIDJSON_UINT64_C2(0xfa000000, 00000000), -54),  // 10^3
            DiyFp(RAPIDJSON_UINT64_C2(0x9c400000, 00000000), -50),  // 10^4
            DiyFp(RAPIDJSON_UINT64_C2(0xc3500000, 00000000), -47),  // 10^5
            DiyFp(RAPIDJSON_UINT64_C2(0xf4240000, 00000000), -44),  // 10^6
            DiyFp(RAPIDJSON_UINT64_C2(0x98968000, 00000000), -40)   // 10^7
        };
        int adjustment = dExp - actualExp - 1;
        RAPIDJSON_ASSERT(adjustment >= 0 && adjustment < 7);
        v = v * kPow10[adjustment];
        if (length + adjustment > 19) // has more digits than decimal digits in 64-bit
            error += kUlp / 2;
    }

    v = v * cachedPower;

    error += kUlp + (error == 0 ? 0 : 1);

    const int oldExp = v.e;
    v = v.Normalize();
    error <<= oldExp - v.e;

    const unsigned effectiveSignificandSize = Double::EffectiveSignificandSize(64 + v.e);
    unsigned precisionSize = 64 - effectiveSignificandSize;
    if (precisionSize + kUlpShift >= 64) {
        unsigned scaleExp = (precisionSize + kUlpShift) - 63;
        v.f >>= scaleExp;
        v.e += scaleExp; 
        error = (error >> scaleExp) + 1 + kUlp;
        precisionSize -= scaleExp;
    }

    DiyFp rounded(v.f >> precisionSize, v.e + precisionSize);
    const uint64_t precisionBits = (v.f & ((uint64_t(1) << precisionSize) - 1)) * kUlp;
    const uint64_t halfWay = (uint64_t(1) << (precisionSize - 1)) * kUlp;
    if (precisionBits >= halfWay + error)
        rounded.f++;

    *result = rounded.ToDouble();

    return halfWay - error >= precisionBits || precisionBits >= halfWay + error;
}

inline double StrtodBigInteger(double approx, const char* decimals, size_t length, size_t decimalPosition, int exp) {
    const BigInteger dInt(decimals, length);
    const int dExp = (int)decimalPosition - (int)length + exp;
    Double a(approx);
    for (int i = 0; i < 10; i++) {
        bool adjustToNegative;
        int cmp = CheckWithinHalfULP(a.Value(), dInt, dExp, &adjustToNegative);
        if (cmp < 0)
            return a.Value();  // within half ULP
        else if (cmp == 0) {
            // Round towards even
            if (a.Significand() & 1)
                return adjustToNegative ? a.PreviousPositiveDouble() : a.NextPositiveDouble();
            else
                return a.Value();
        }
        else // adjustment
            a = adjustToNegative ? a.PreviousPositiveDouble() : a.NextPositiveDouble();
    }

    // This should not happen, but in case there is really a bug, break the infinite-loop
    return a.Value();
}

inline double StrtodFullPrecision(double d, int p, const char* decimals, size_t length, size_t decimalPosition, int exp) {
    RAPIDJSON_ASSERT(d >= 0.0);
    RAPIDJSON_ASSERT(length >= 1);

    double result;
    if (StrtodFast(d, p, &result))
        return result;

    // Trim leading zeros
    while (*decimals == '0' && length > 1) {
        length--;
        decimals++;
        decimalPosition--;
    }

    // Trim trailing zeros
    while (decimals[length - 1] == '0' && length > 1) {
        length--;
        decimalPosition--;
        exp++;
    }

    // Trim right-most digits
    const int kMaxDecimalDigit = 780;
    if ((int)length > kMaxDecimalDigit) {
        exp += (int(length) - kMaxDecimalDigit);
        length = kMaxDecimalDigit;
    }

    // If too small, underflow to zero
    if (int(length) + exp < -324)
        return 0.0;

    if (StrtodDiyFp(decimals, length, decimalPosition, exp, &result))
        return result;

    // Use approximation from StrtodDiyFp and make adjustment with BigInteger comparison
    return StrtodBigInteger(result, decimals, length, decimalPosition, exp);
}

} // namespace internal
RAPIDJSON_NAMESPACE_END

#endif // RAPIDJSON_STRTOD_
