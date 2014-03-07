//  Copyright (C) 2007 Douglas Gregor
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_CXX11_STATIC_ASSERT
//  TITLE:         C++0x static_assert unavailable
//  DESCRIPTION:   The compiler does not support C++0x static assertions

namespace boost_no_cxx11_static_assert {

int test()
{
   static_assert(true, "OK");
   return 0;
}

}
