///////////////////////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/multiprecision/cpp_int.hpp>
#if defined(HAVE_FLOAT128)
#include <boost/multiprecision/float128.hpp>
#endif

#ifndef BOOST_NO_CXX11_CONSTEXPR

template <class T>
void silence_unused(const T&){}

template <class T>
void test1()
{
   constexpr T i1 = 2u;
   constexpr T i2;
   constexpr T i3 = -3;
   constexpr T i4(i1);

   silence_unused(i1);
   silence_unused(i2);
   silence_unused(i3);
   silence_unused(i4);
}
template <class T>
void test2()
{
   constexpr T i1 = 2u;
   constexpr T i2;
   constexpr T i3 = -3;

   silence_unused(i1);
   silence_unused(i2);
   silence_unused(i3);
}
template <class T>
void test3()
{
   constexpr T i1 = 2u;
   constexpr T i2;

   silence_unused(i1);
   silence_unused(i2);
}

using namespace boost::multiprecision;

template void test1<number<cpp_int_backend<64, 64, unsigned_magnitude, unchecked, void>, et_off> >();
template void test1<number<cpp_int_backend<64, 64, signed_magnitude, unchecked, void>, et_off> >();
template void test3<number<cpp_int_backend<2048, 2048, unsigned_magnitude, unchecked, void>, et_off> >();
template void test2<number<cpp_int_backend<2048, 2048, signed_magnitude, unchecked, void>, et_off> >();

#if defined(HAVE_FLOAT128)
template void test1<float128>();
#endif

#endif
