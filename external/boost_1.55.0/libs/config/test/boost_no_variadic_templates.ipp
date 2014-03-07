//  Copyright (C) 2007 Douglas Gregor
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_CXX11_VARIADIC_TEMPLATES
//  TITLE:         C++0x variadic templates unavailable
//  DESCRIPTION:   The compiler does not support C++0x variadic templates

namespace boost_no_cxx11_variadic_templates {

template<typename... Elements> struct tuple {};

int test()
{
   return 0;
}

}
