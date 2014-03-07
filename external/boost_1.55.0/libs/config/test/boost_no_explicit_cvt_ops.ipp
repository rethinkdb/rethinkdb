//  (C) Copyright Beman Dawes 2008

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS 
//  TITLE:         C++0x explicit conversion operators unavailable
//  DESCRIPTION:   The compiler does not support C++0x explicit conversion operators

#if defined(__GNUC__) && !defined(__GXX_EXPERIMENTAL_CXX0X__) && !defined(BOOST_INTEL_STDCXX0X)
#  error This feature is not available in non-C++0x mode
#endif

namespace boost_no_cxx11_explicit_conversion_operators {

void quiet_warning(int){}

  struct foo {
    explicit operator int() { return 1; }
  };

int test()
{
  foo f;
  int i = int(f);
  quiet_warning(i);
  return 0;
}

}
