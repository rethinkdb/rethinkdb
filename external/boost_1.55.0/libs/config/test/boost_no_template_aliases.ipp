//  (C) Copyright Beman Dawes 2009

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_TEMPLATE_ALIASES
//  TITLE:         C++0x template_aliases feature unavailable
//  DESCRIPTION:   The compiler does not support the C++0x template_aliases feature

namespace boost_no_cxx11_template_aliases {

using PINT = void (*)(int);             // using plus C-style type

int test()
{
  return 0;
}

}
