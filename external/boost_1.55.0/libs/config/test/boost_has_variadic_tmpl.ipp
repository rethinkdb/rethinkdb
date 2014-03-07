//  Copyright (C) 2007 Douglas Gregor
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_VARIADIC_TMPL
//  TITLE:         variadic templates
//  DESCRIPTION:   The compiler supports C++0x variadic templates

namespace boost_has_variadic_tmpl {

template<typename... Elements> struct tuple {};

int test()
{
   return 0;
}

}
