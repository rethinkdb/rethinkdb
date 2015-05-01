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

#ifndef RAPIDJSON_IEEE754_
#define RAPIDJSON_IEEE754_

#include "../rapidjson.h"

RAPIDJSON_NAMESPACE_BEGIN
namespace internal {

class Double {
public:
    Double() {}
    Double(double d) : d(d) {}
    Double(uint64_t u) : u(u) {}

    double Value() const { return d; }
    uint64_t Uint64Value() const { return u; }

    double NextPositiveDouble() const {
        RAPIDJSON_ASSERT(!Sign());
        return Double(u + 1).Value();
    }

    double PreviousPositiveDouble() const {
        RAPIDJSON_ASSERT(!Sign());
        if (d == 0.0)
            return 0.0;
        else
            return Double(u - 1).Value();
    }

    bool Sign() const { return (u & kSignMask) != 0; }
    uint64_t Significand() const { return u & kSignificandMask; }
    int Exponent() const { return ((u & kExponentMask) >> kSignificandSize) - kExponentBias; }

    bool IsNan() const { return (u & kExponentMask) == kExponentMask && Significand() != 0; }
    bool IsInf() const { return (u & kExponentMask) == kExponentMask && Significand() == 0; }
    bool IsNormal() const { return (u & kExponentMask) != 0 || Significand() == 0; }

    uint64_t IntegerSignificand() const { return IsNormal() ? Significand() | kHiddenBit : Significand(); }
    int IntegerExponent() const { return (IsNormal() ? Exponent() : kDenormalExponent) - kSignificandSize; }
    uint64_t ToBias() const { return (u & kSignMask) ? ~u + 1 : u | kSignMask; }

    static unsigned EffectiveSignificandSize(int order) {
        if (order >= -1021)
            return 53;
        else if (order <= -1074)
            return 0;
        else
            return order + 1074;
    }

private:
    static const int kSignificandSize = 52;
    static const int kExponentBias = 0x3FF;
    static const int kDenormalExponent = 1 - kExponentBias;
    static const uint64_t kSignMask = RAPIDJSON_UINT64_C2(0x80000000, 0x00000000);
    static const uint64_t kExponentMask = RAPIDJSON_UINT64_C2(0x7FF00000, 0x00000000);
    static const uint64_t kSignificandMask = RAPIDJSON_UINT64_C2(0x000FFFFF, 0xFFFFFFFF);
    static const uint64_t kHiddenBit = RAPIDJSON_UINT64_C2(0x00100000, 0x00000000);

    union {
        double d;
        uint64_t u;
    };
};

} // namespace internal
RAPIDJSON_NAMESPACE_END

#endif // RAPIDJSON_IEEE754_
