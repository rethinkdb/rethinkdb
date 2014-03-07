//  (C) Copyright John Maddock 2013

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_USER_DEFINED_LITERALS
//  TITLE:         C++11 user defined literals.
//  DESCRIPTION:   The compiler does not support the C++11 literals including user-defined suffixes.

#include <memory>

namespace boost_no_cxx11_user_defined_literals {

struct my_literal
{
   constexpr my_literal() : val(0) {}
   constexpr my_literal(int i) : val(i) {}
   constexpr my_literal(const my_literal& a) : val(a.val) {}
   constexpr bool operator==(const my_literal& a) { return val == a.val; }
   int val;
};

template <unsigned base, unsigned long long val, char... Digits>
struct parse_int
{
    // The default specialization is also the termination condition:
    // it gets invoked only when sizeof...Digits == 0.
    static_assert(base<=16u,"only support up to hexadecimal");
    static constexpr unsigned long long value{ val };
};

template <unsigned base, unsigned long long val, char c, char... Digits>
struct parse_int<base, val, c, Digits...>
{
    static constexpr unsigned long long char_value = (c >= '0' && c <= '9')
            ? c - '0'
            : (c >= 'a' && c <= 'f')
            ? c - 'a'
            : (c >= 'A' && c <= 'F')
            ? c - 'A'
            : 400u;
    static_assert(char_value < base, "Encountered a digit out of range");
    static constexpr unsigned long long value{ parse_int<base, val * base +
char_value, Digits...>::value };
};

constexpr my_literal operator "" _suf1(unsigned long long v)
{
   return my_literal(v);
}
template <char...PACK>
constexpr my_literal operator "" _bin()
{
   return parse_int<2, 0, PACK...>::value;
}

int test()
{
   constexpr my_literal a = 0x23_suf1;
   constexpr my_literal b = 1001_bin;
   return ((a == my_literal(0x23)) && (b == my_literal(9))) ? 0 : 1;
}

}
