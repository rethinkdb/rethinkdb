
//  (C) Copyright Beman Dawes 2008

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_DECLTYPE
//  TITLE:         C++0x decltype unavailable
//  DESCRIPTION:   The compiler does not support C++0x decltype

namespace boost_no_cxx11_decltype {

void quiet_warning(int){}

struct test_class
{
   test_class() {}
};

test_class get_test_class()
{
   return test_class();
}

template<typename F>
void baz(F f)
{
    //
    // Strangely VC-10 deduces the return type of F
    // to be "test_class&".  Remove the constructor
    // from test_class and then decltype does work OK!!
    //
    typedef decltype(f()) res;
    res r;
}

int test()
{
  int i;
  decltype(i) j(0);
  quiet_warning(j);
  decltype(get_test_class()) k;
  #ifndef _MSC_VER
  // Although the VC++ decltype is buggy, we none the less enable support,
  // so don't test the bugs for now!
  baz(get_test_class);
  #endif
  return 0;
}

}
