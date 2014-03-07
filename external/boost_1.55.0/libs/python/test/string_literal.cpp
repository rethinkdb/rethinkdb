// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/detail/string_literal.hpp>
//#include <stdio.h>

#include <boost/detail/lightweight_test.hpp>
#include <boost/static_assert.hpp>

using namespace boost::python::detail;
    

template <class T>
void expect_string_literal(T const&)
{
    BOOST_STATIC_ASSERT(is_string_literal<T const>::value);
}

int main()
{
    expect_string_literal("hello");
    BOOST_STATIC_ASSERT(!is_string_literal<int*&>::value);
    BOOST_STATIC_ASSERT(!is_string_literal<int* const&>::value);
    BOOST_STATIC_ASSERT(!is_string_literal<int*volatile&>::value);
    BOOST_STATIC_ASSERT(!is_string_literal<int*const volatile&>::value);
    
    BOOST_STATIC_ASSERT(!is_string_literal<char const*>::value);
    BOOST_STATIC_ASSERT(!is_string_literal<char*>::value);
    BOOST_STATIC_ASSERT(!is_string_literal<char*&>::value);
    BOOST_STATIC_ASSERT(!is_string_literal<char* const&>::value);
    BOOST_STATIC_ASSERT(!is_string_literal<char*volatile&>::value);
    BOOST_STATIC_ASSERT(!is_string_literal<char*const volatile&>::value);
    
    BOOST_STATIC_ASSERT(!is_string_literal<char[20]>::value);
    BOOST_STATIC_ASSERT(is_string_literal<char const[20]>::value);
    BOOST_STATIC_ASSERT(is_string_literal<char const[3]>::value);

    BOOST_STATIC_ASSERT(!is_string_literal<int[20]>::value);
    BOOST_STATIC_ASSERT(!is_string_literal<int const[20]>::value);
    BOOST_STATIC_ASSERT(!is_string_literal<int const[3]>::value);
    return boost::report_errors();
}
