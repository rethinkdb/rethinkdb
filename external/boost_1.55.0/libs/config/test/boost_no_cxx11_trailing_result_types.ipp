//  (C) Copyright Andrey Semashev 2013

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_TRAILING_RESULT_TYPES
//  TITLE:         C++11 trailing function result types syntax.
//  DESCRIPTION:   The compiler does not support the new C++11 function result types specification syntax.

namespace boost_no_cxx11_trailing_result_types {

template< typename T >
auto foo(T const& t) -> T
{
    return t;
}

int test()
{
    return foo(0);
}

}
