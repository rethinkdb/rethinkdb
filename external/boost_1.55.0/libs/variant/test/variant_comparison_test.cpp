//-----------------------------------------------------------------------------
// boost-libs variant/test/variant_comparison_test.cpp source file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2003
// Eric Friedman, Itay Maman
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "boost/variant/variant.hpp"
#include "boost/test/minimal.hpp"

#include <iostream>
#include <sstream>
#include <string>

#include <algorithm>
#include <vector>

#include "boost/detail/workaround.hpp"
#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x0551))
#    pragma warn -lvc // temporary used for parameter warning
#endif

template <typename T>
void assert_equality_comparable(
      const T& x, const T& y, const T& z
    )
{
    // identity check
    BOOST_CHECK( !(&x == &y) || (x == y) );
    BOOST_CHECK( !(&x == &z) || (x == z) );
    BOOST_CHECK( !(&y == &z) || (y == z) );

    // reflexivity check
    BOOST_CHECK( (x == x) );
    BOOST_CHECK( (y == y) );
    BOOST_CHECK( (z == z) );

    // symmetry check
    BOOST_CHECK( !(x == y) || (y == x) );
    BOOST_CHECK( !(y == x) || (x == y) );

    BOOST_CHECK( !(x == z) || (z == x) );
    BOOST_CHECK( !(z == x) || (x == z) );

    BOOST_CHECK( !(y == z) || (z == y) );
    BOOST_CHECK( !(z == y) || (y == z) );

    // transitivity check
    BOOST_CHECK( !(x == y && y == z) || (x == z) );
    BOOST_CHECK( !(x == z && z == y) || (x == y) );
    BOOST_CHECK( !(y == z && z == x) || (y == x) );
}

template <typename T>
void assert_less_than_comparable(
      const T& x, const T& y, const T& z
    )
{
    // irreflexivity check
    BOOST_CHECK( !(x < x) );
    BOOST_CHECK( !(y < y) );
    BOOST_CHECK( !(z < z) );

    // transitivity check
    BOOST_CHECK( (x < y) );
    BOOST_CHECK( (y < z) );
    BOOST_CHECK( (x < z) );

    // antisymmetry check
    BOOST_CHECK( !(y < x) );
    BOOST_CHECK( !(z < y) );
    BOOST_CHECK( !(z < x) );
}

template <typename It>
std::string print_range(It first, It last)
{
    std::ostringstream result;

    while (first != last)
    {
        result << *first << ' ';
        ++first;
    }

    return result.str();
}

int test_main(int , char* [])
{
    typedef boost::variant<int, std::string> var_t;
    
    var_t var1(3);
    var_t var2(5);
    var_t var3("goodbye");
    var_t var4("hello");

    assert_equality_comparable(var1, var1, var1);
    assert_equality_comparable(var_t(var1), var_t(var1), var_t(var1));
    assert_equality_comparable(var1, var2, var3);

    assert_less_than_comparable(var1, var2, var3);
    assert_less_than_comparable(var2, var3, var4);

    std::vector<var_t> vec;
    vec.push_back( var3 );
    vec.push_back( var2 );
    vec.push_back( var4 );
    vec.push_back( var1 );
    std::sort(vec.begin(), vec.end());

    std::string sort_result( print_range(vec.begin(), vec.end()) );
    BOOST_CHECK( sort_result == "3 5 goodbye hello " );

    return boost::exit_success;
}
