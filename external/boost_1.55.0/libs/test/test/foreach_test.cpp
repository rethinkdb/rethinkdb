//  (C) Copyright Gennadiy Rozental 2001-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 57993 $
//
//  Description : BOOST_TEST_FOREACH compile only test
// *****************************************************************************

// STL
#include <iostream>
#include <list>
#include <vector>
#include <string>

#include <boost/test/utils/foreach.hpp>

#ifdef BOOST_MSVC
#pragma warning(disable:4702) // Unreachable code
#endif

template<class T>
void baz( std::list<T>const& list_of_T )
{
    // an example of using BOOST_TEST_FOREACH with dependent types
    BOOST_TEST_FOREACH( T const&, t, list_of_T )
    {
        std::cout << t << std::endl;
    }
}

int main()
{
    std::list<int> int_list;
    int_list.push_back( 1 );
    int_list.push_back( 2 );
    int_list.push_back( 3 );

    // use BOOST_TEST_FOREACH with a STL container, and a reference as the loop variable.
    BOOST_TEST_FOREACH( int&, i, int_list )
    {
        ++i;
        std::cout << i << std::endl;
    }

    std::cout << std::endl;

    // use BOOST_TEST_FOREACH with a std::vector
    std::vector<int> int_vec;
    int_vec.push_back( 1 );
    int_vec.push_back( 2 );
    int_vec.push_back( 3 );
    int_vec.push_back( 3 );
    int_vec.push_back( 3 );

    BOOST_TEST_FOREACH( int const&, i, int_vec )
    {
        std::cout << i << std::endl;
        if( i == 3 )
            break;
    }

    std::cout << std::endl;

    // use BOOST_TEST_FOREACH with dependent types
    baz( int_list );
    std::cout << std::endl;

    // iterate over characters in a std::string
    std::string str( "hello" );

    BOOST_TEST_FOREACH( char&, ch, str )
    {
        std::cout << ch;
        // mutate the string
        ++ch;
    }
    std::cout << std::endl;
    std::cout << std::endl;

    BOOST_TEST_FOREACH( char, ch, str )
    {
        // break work as you would expect
        std::cout << ch;
        break;
    }
    std::cout << std::endl;
    std::cout << std::endl;

    BOOST_TEST_FOREACH( char, ch, str )
    {
        // continue work as you would expect
        if( ch == 'm' )
            continue;

        std::cout << ch;
    }
    std::cout << std::endl;
    std::cout << std::endl;

    // use BOOST_TEST_FOREACH with const reference.
    std::vector<int> const& int_vec_const_ref = int_vec;

    BOOST_TEST_FOREACH( int const&, i, int_vec_const_ref )
    {
        std::cout << (i+1) << std::endl;
    }
    std::cout << std::endl;

    return 0;
}
