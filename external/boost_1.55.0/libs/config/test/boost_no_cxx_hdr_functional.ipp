//  (C) Copyright John Maddock 2012

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_HDR_FUNCTIONAL
//  TITLE:         C++11 <functional> unavailable
//  DESCRIPTION:   The compiler does not support the C++11 features added to <functional>

#include <functional>

void f(int, float){}

namespace boost_no_cxx11_hdr_functional {

int test()
{
   int i = 0;
   std::ref(i);
   std::cref(i);

   std::bit_and<int> b1;
   std::bit_or<int>  b2;
   std::bit_xor<int> b3;

   std::hash<short> hs;

   (void)b1;
   (void)b2;
   (void)b3;
   (void)hs;

   std::bind(f, std::placeholders::_1, 0.0f);

   std::function<void(int, float)> fun(f);

   return 0;
}

}
