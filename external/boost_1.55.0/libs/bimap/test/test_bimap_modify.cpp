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

struct id{};

void test_bimap_modify()
{
    using namespace boost::bimaps;

    typedef bimap<int,long> bm;

    bm b;
    b.insert( bm::value_type(2,200) );

    BOOST_CHECK( b.left.at(2) == 200 );

    bool result;

    // replace
    //----------------------------------------------------------------------

    // successful replace in left map view
    {
        bm::left_iterator i = b.left.begin();

        result = b.left.replace( i, bm::left_value_type(1,100) );

        BOOST_CHECK( result );
        BOOST_CHECK( b.size() == 1 );
        BOOST_CHECK( i->first == 1 && i->second == 100 );
        BOOST_CHECK( b.left.at(1) == 100 );

        result = b.left.replace_key( i, 2 );

        BOOST_CHECK( result );
        BOOST_CHECK( b.size() == 1 );
        BOOST_CHECK( i->first == 2 && i->second == 100 );
        BOOST_CHECK( b.left.at(2) == 100 );

        result = b.left.replace_data( i, 200 );

        BOOST_CHECK( result );
        BOOST_CHECK( b.size() == 1 );
        BOOST_CHECK( i->first == 2 && i->second == 200 );
        BOOST_CHECK( b.left.at(2) == 200 );
    }

    // successful replace in right map view
    {
        bm::right_iterator i = b.right.begin();

        result = b.right.replace( i, bm::right_value_type(100,1) );

        BOOST_CHECK( result );
        BOOST_CHECK( b.size() == 1 );
        BOOST_CHECK( i->first == 100 && i->second == 1 );
        BOOST_CHECK( b.right.at(100) == 1 );

        result = b.right.replace_key( i, 200 );

        BOOST_CHECK( result );
        BOOST_CHECK( b.size() == 1 );
        BOOST_CHECK( i->first == 200 && i->second == 1 );
        BOOST_CHECK( b.right.at(200) == 1 );

        result = b.right.replace_data( i, 2 );

        BOOST_CHECK( result );
        BOOST_CHECK( b.size() == 1 );
        BOOST_CHECK( i->first == 200 && i->second == 2 );
        BOOST_CHECK( b.right.at(200) == 2 );
    }

    // successful replace in set of relations view
    {
        bm::iterator i = b.begin();

        result = b.replace( i, bm::value_type(1,100) );

        BOOST_CHECK( result );
        BOOST_CHECK( b.size() == 1 );
        BOOST_CHECK( i->left == 1 && i->right == 100 );
        BOOST_CHECK( b.left.at(1) == 100 );

        result = b.replace_left( i, 2 );

        BOOST_CHECK( result );
        BOOST_CHECK( b.size() == 1 );
        BOOST_CHECK( i->left == 2 && i->right == 100 );
        BOOST_CHECK( b.left.at(2) == 100 );

        result = b.replace_right( b.begin(), 200 );

        BOOST_CHECK( result );
        BOOST_CHECK( b.size() == 1 );
        BOOST_CHECK( i->left == 2 && i->right == 200 );
        BOOST_CHECK( b.left.at(2) == 200 );

    }

    b.clear();
    b.insert( bm::value_type(1,100) );
    b.insert( bm::value_type(2,200) );

    // fail to replace in left map view
    {
        bm::left_iterator i = b.left.begin();

        result = b.left.replace( i, bm::left_value_type(2,100) );

        BOOST_CHECK( ! result );
        BOOST_CHECK( b.size() == 2 );
        BOOST_CHECK( i->first == 1 && i->second == 100 );
        BOOST_CHECK( b.left.at(1) == 100 );
        BOOST_CHECK( b.left.at(2) == 200 );


        // Add checks for replace_key and replace_data
    }

    // Add checks for fail to replace in right map view

    // Add checks for fail to replace in set of relations view


    // modify
    // ----------------------------------------------------------------------

    b.clear();
    b.insert( bm::value_type(1,100) );

    // successful modify in left map view
    {
        result = b.left.modify_key( b.left.begin(), _key = 2 );

        BOOST_CHECK( result );
        BOOST_CHECK( b.size() == 1 );
        BOOST_CHECK( b.left.at(2) == 100 );

        result = b.left.modify_data( b.left.begin() , _data = 200 );

        BOOST_CHECK( result );
        BOOST_CHECK( b.size() == 1 );
        BOOST_CHECK( b.left.at(2) == 200 );
    }

    // Add checks for successful modify in right map view

    // Add checks for fails to modify in left map view


}

void test_bimap_replace_with_info() 
{
    using namespace boost::bimaps;
    typedef bimap<int,long,with_info<int> > bm;

    bm b;
    b.insert( bm::value_type(2,200,-2) );

    BOOST_CHECK( b.left.at(2)      == 200 );
    BOOST_CHECK( b.left.info_at(2) ==  -2 );
 
    // Use set view
    {
        bm::iterator i = b.begin();

        bool result = b.replace( i, bm::value_type(1,100,-1) );

        BOOST_CHECK( result );
        BOOST_CHECK( b.size() == 1 );
        BOOST_CHECK( i->left == 1 && i->right == 100 );
        BOOST_CHECK( i->info == -1 );
        
        result = b.replace_left( i, 2 );

        BOOST_CHECK( result );
        BOOST_CHECK( b.size() == 1 );
        BOOST_CHECK( i->left == 2 && i->right == 100 );
        BOOST_CHECK( i->info == -1 );
        
        result = b.replace_right( i, 200 );

        BOOST_CHECK( result );
        BOOST_CHECK( b.size() == 1 );
        BOOST_CHECK( i->left == 2 && i->right == 200 );
        BOOST_CHECK( i->info == -1 );
    }

    // Use map view
    {
        bm::left_iterator i = b.left.begin();

        bool result = b.left.replace( i, bm::left_value_type(1,100,-1) );

        BOOST_CHECK( result );
        BOOST_CHECK( b.left.size() == 1 );
        BOOST_CHECK( i->first == 1 && i->second == 100 );
        BOOST_CHECK( i->info == -1 );
        
        result = b.left.replace_key( i, 2 );

        BOOST_CHECK( result );
        BOOST_CHECK( b.left.size() == 1 );
        BOOST_CHECK( i->first == 2 && i->second == 100 );
        BOOST_CHECK( i->info == -1 );
        
        result = b.left.replace_data( i, 200 );

        BOOST_CHECK( result );
        BOOST_CHECK( b.left.size() == 1 );
        BOOST_CHECK( i->first == 2 && i->second == 200 );
        BOOST_CHECK( i->info == -1 );
    }
}

int test_main( int, char* [] )
{
    test_bimap_modify();

    test_bimap_replace_with_info();
    
    return 0;
}

