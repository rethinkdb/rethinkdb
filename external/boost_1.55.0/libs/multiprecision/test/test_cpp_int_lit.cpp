///////////////////////////////////////////////////////////////////////////////
//  Copyright 2013 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/multiprecision/cpp_int.hpp>
#include "test.hpp"


#ifdef BOOST_MP_USER_DEFINED_LITERALS


using namespace boost::multiprecision;

template <class T>
void test_literal(T val, const char* p)
{
   BOOST_CHECK_EQUAL(val, cpp_int(p));
}

#define TEST_LITERAL(x)\
{\
   constexpr auto val1 = BOOST_JOIN(x, _cppi);\
   constexpr int1024_t val2 = BOOST_JOIN(x, _cppi1024);\
   constexpr auto val3 = BOOST_JOIN(x, _cppui);\
   constexpr uint1024_t val4 = BOOST_JOIN(x, _cppui1024);\
   test_literal(val1, BOOST_STRINGIZE(x));\
   test_literal(val2, BOOST_STRINGIZE(x));\
   test_literal(val3, BOOST_STRINGIZE(x));\
   test_literal(val4, BOOST_STRINGIZE(x));\
   /* Negative values: */\
   constexpr auto val5 = -BOOST_JOIN(x, _cppi);\
   constexpr int1024_t val6 = -BOOST_JOIN(x, _cppi1024);\
   constexpr auto val7 = -val1;\
   constexpr int1024_t val8 = -val2;\
   BOOST_CHECK_EQUAL(val5, -cpp_int(val1));\
   BOOST_CHECK_EQUAL(val6, -cpp_int(val1));\
   BOOST_CHECK_EQUAL(val7, -cpp_int(val1));\
   BOOST_CHECK_EQUAL(val8, -cpp_int(val1));\
}\

int main()
{
   using namespace boost::multiprecision::literals;
   TEST_LITERAL(0x0);
   TEST_LITERAL(0x00000);
   TEST_LITERAL(0x10000000);
   TEST_LITERAL(0x1234500000000123450000000123345000678000000456000000567000000fefabc00000000000000);
   return boost::report_errors();
}

#else

int main() { return 0; }


#endif
