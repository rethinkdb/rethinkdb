/* tests for using class array<> specialization for size 0
 * (C) Copyright Alisdair Meredith 2006.
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include <string>
#include <iostream>
#include <boost/array.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

namespace {

template< class T >
void    BadValue( const T &  )
{
    BOOST_CHECK ( false );
}

template< class T >
void    RunTests()
{
    typedef boost::array< T, 0 >    test_type;

    //  Test value and aggegrate initialization
    test_type                   test_case   =   {};
    const boost::array< T, 0 >  const_test_case = test_type();

    test_case.fill ( T() );

    //  front/back and operator[] must compile, but calling them is undefined
    //  Likewise, all tests below should evaluate to false, avoiding undefined behaviour
    BOOST_CHECK (       test_case.empty());
    BOOST_CHECK ( const_test_case.empty());

    BOOST_CHECK (       test_case.size() == 0 );
    BOOST_CHECK ( const_test_case.size() == 0 );

    //  Assert requirements of TR1 6.2.2.4
    BOOST_CHECK ( test_case.begin()  == test_case.end());
    BOOST_CHECK ( test_case.cbegin() == test_case.cend());
    BOOST_CHECK ( const_test_case.begin() == const_test_case.end());
    BOOST_CHECK ( const_test_case.cbegin() == const_test_case.cend());

    BOOST_CHECK ( test_case.begin() != const_test_case.begin() );
    if( test_case.data() == const_test_case.data() ) {
    //  Value of data is unspecified in TR1, so no requirement this test pass or fail
    //  However, it must compile!
    }

    //  Check can safely use all iterator types with std algorithms
    std::for_each( test_case.begin(), test_case.end(), BadValue< T > );
    std::for_each( test_case.rbegin(), test_case.rend(), BadValue< T > );
    std::for_each( test_case.cbegin(), test_case.cend(), BadValue< T > );
    std::for_each( const_test_case.begin(), const_test_case.end(), BadValue< T > );
    std::for_each( const_test_case.rbegin(), const_test_case.rend(), BadValue< T > );
    std::for_each( const_test_case.cbegin(), const_test_case.cend(), BadValue< T > );

    //  Check swap is well formed
    std::swap( test_case, test_case );

    //  Check assignment operator and overloads are well formed
    test_case   =   const_test_case;

    //  Confirm at() throws the std lib defined exception
    try {
        BadValue( test_case.at( 0 ));
    } catch ( const std::out_of_range & ) {
    }

    try {
        BadValue( const_test_case.at( 0 ) );
    } catch ( const std::out_of_range & ) {
    }
}

}

BOOST_AUTO_TEST_CASE( test_main )
{
    RunTests< bool >();
    RunTests< void * >();
    RunTests< long double >();
    RunTests< std::string >();
}

