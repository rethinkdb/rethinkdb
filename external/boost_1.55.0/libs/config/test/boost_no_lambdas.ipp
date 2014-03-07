//  (C) Copyright Beman Dawes 2009

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_LAMBDAS
//  TITLE:         C++0x lambda feature unavailable
//  DESCRIPTION:   The compiler does not support the C++0x lambda feature

#if defined(__GNUC__) && !defined(__GXX_EXPERIMENTAL_CXX0X__) && !defined(BOOST_INTEL_STDCXX0X)
#  error This feature is not available in non-C++0x mode
#endif

namespace boost_no_cxx11_lambdas {

int test()
{
  [](){};
  return 0;
}

}
