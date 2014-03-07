//  Copyright (C) 2009 Andrey Semashev
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_CXX11_AUTO_MULTIDECLARATIONS
//  TITLE:         C++0x auto multideclarators unavailable
//  DESCRIPTION:   The compiler does not support C++0x declarations of series of variables with automatically deduced type

namespace boost_no_cxx11_auto_multideclarations {

void check_f(int&, int*&)
{
}

int test()
{
   auto x = 10, *y = &x;
   check_f(x, y);
   return 0;
}

}
