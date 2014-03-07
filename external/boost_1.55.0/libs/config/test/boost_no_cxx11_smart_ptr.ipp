//  (C) Copyright John Maddock 2012

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_SMART_PTR
//  TITLE:         C++11 <memory> has no shared_ptr and unique_ptr
//  DESCRIPTION:   The compiler does not support the C++11 smart pointer features added to <memory>

#include <memory>
// Hash functions for shared pointers should be in <memory>
// but with some std lib's we have to include <functional> as well...
#include <functional>

namespace boost_no_cxx11_smart_ptr {

int test()
{
   std::unique_ptr<int> upi(new int);
   std::shared_ptr<int> spi(new int), spi2(new int);
   spi = std::static_pointer_cast<int>(spi);

   std::hash<std::shared_ptr<int> > h1;
   std::hash<std::unique_ptr<int> > h2;

   (void)h1;
   (void)h2;

   return 0;
}

}
