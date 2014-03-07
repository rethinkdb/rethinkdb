//  (C) Copyright Beman Dawes 2009

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_HDR_TYPEINDEX
//  TITLE:         C++0x header <typeindex> unavailable
//  DESCRIPTION:   The standard library does not supply C++0x header <typeindex>

#include <typeindex>

namespace boost_no_cxx11_hdr_typeindex {

int test()
{
  std::type_index t1 = typeid(int);
  std::type_index t2 = typeid(double);
  std::hash<std::type_index> h;
  return (t1 != t2) && (h(t1) != h(t2)) ? 0 : 1;
}

}
