// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  VC++ 8.0 warns on usage of certain Standard Library and API functions that
//  can be cause buffer overruns or other possible security issues if misused.
//  See http://msdn.microsoft.com/msdnmag/issues/05/05/SafeCandC/default.aspx
//  But the wording of the warning is misleading and unsettling, there are no
//  portable alternative functions, and VC++ 8.0's own libraries use the
//  functions in question. So turn off the warnings.
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

#include <boost/config.hpp>

// Boost.Test
#include <boost/test/minimal.hpp>

// Boost.Bimap
#include <boost/bimap/support/lambda.hpp>
#include <boost/bimap/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/bimap/list_of.hpp>
#include <boost/bimap/vector_of.hpp>
#include <boost/bimap/unconstrained_set_of.hpp>


void test_bimap_operator_bracket()
{
    using namespace boost::bimaps;

    // Simple test
    {
        typedef bimap< int, std::string > bm;

        bm b;
        b.insert( bm::value_type(0,"0") );
        b.insert( bm::value_type(1,"1") );
        b.insert( bm::value_type(2,"2") );
        b.insert( bm::value_type(3,"3") );

        BOOST_CHECK( b.left.at(1) == "1" );

        // Out of range test
        {
            bool value_not_found_test_passed = false;
            b.clear();
            try
            {
                bool comp;
                comp = b.left.at(2) < "banana";
            }
            catch( std::out_of_range & )
            {
                value_not_found_test_passed = true;
            }

            BOOST_CHECK( value_not_found_test_passed );
        }
    }

    // Mutable data test (1)
    {
        typedef bimap<int, list_of<std::string> > bm;
        bm b;

        // Out of range test
        {
            bool value_not_found_test_passed = false;
            b.clear();
            try
            {
                bool comp;
                comp = b.left.at(1) < "banana";
            }
            catch( std::out_of_range & )
            {
                value_not_found_test_passed = true;
            }

            BOOST_CHECK( value_not_found_test_passed );

        }

        // Out of range test (const version)
        {
            bool value_not_found_test_passed = false;
            b.clear();
            try
            {
                const bm & cb(b);
                bool comp;
                comp = cb.left.at(1) < "banana";
            }
            catch( std::out_of_range & )
            {
                value_not_found_test_passed = true;
            }

            BOOST_CHECK( value_not_found_test_passed );
        }

        BOOST_CHECK( b.left[1]    == "" );
        BOOST_CHECK( b.left.at(1) == "" );
        b.left[2] = "two";
        BOOST_CHECK( b.left.at(2) == "two" );
        b.left[2] = "<<two>>";
        BOOST_CHECK( b.left.at(2) == "<<two>>" );
        b.left.at(2) = "two";
        BOOST_CHECK( b.left.at(2) == "two" );

    }

    // Mutable data test (2)
    {
        typedef bimap< vector_of<int>, unordered_set_of<std::string> > bm;
        bm b;

        // Out of range test
        {
            bool value_not_found_test_passed = false;
            b.clear();
            try
            {
                bool comp;
                comp = b.right.at("banana") < 1;
            }
            catch( std::out_of_range & )
            {
                value_not_found_test_passed = true;
            }
            BOOST_CHECK( value_not_found_test_passed );
        }

        // Out of range test (const version)
        {
            bool value_not_found_test_passed = false;
            b.clear();
            try
            {
                const bm & cb(b);
                bool comp;
                comp = cb.right.at("banana") < 1;
            }
            catch( std::out_of_range & )
            {
                value_not_found_test_passed = true;
            }

            BOOST_CHECK( value_not_found_test_passed );
        }

        b.right["one"];
        BOOST_CHECK( b.size() == 1 );
        b.right["two"] = 2;
        BOOST_CHECK( b.right.at("two") == 2 );
        b.right["two"] = -2;
        BOOST_CHECK( b.right.at("two") == -2 );
        b.right.at("two") = 2;
        BOOST_CHECK( b.right.at("two") == 2 );
    }

    // Mutable data test (3)
    {
        typedef bimap< unconstrained_set_of<int>,
                       unordered_set_of<std::string>,
                       right_based                       > bm;

        bm b;

        b.right["one"];
        BOOST_CHECK( b.size() == 1 );
        b.right["two"] = 2;
        BOOST_CHECK( b.right.at("two") == 2 );
        b.right["two"] = -2;
        BOOST_CHECK( b.right.at("two") == -2 );
        b.right.at("two") = 2;
        BOOST_CHECK( b.right.at("two") == 2 );
    }

}

int test_main( int, char* [] )
{
    test_bimap_operator_bracket();

    return 0;
}

