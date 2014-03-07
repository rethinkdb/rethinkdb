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

// Boost.Bimap Example
//-----------------------------------------------------------------------------

#include <boost/config.hpp>

#include <string>
#include <iostream>

#include <boost/bimap/bimap.hpp>
#include <boost/bimap/support/lambda.hpp>

using namespace boost::bimaps;

void test_replace()
{
    //[ code_tutorial_replace

    typedef bimap< int, std::string > bm_type;
    bm_type bm;

    bm.insert( bm_type::value_type(1,"one") );

    // Replace (1,"one") with (1,"1") using the right map view
    {
        bm_type::right_iterator it = bm.right.find("one");

        bool successful_replace = bm.right.replace_key( it, "1" );

        assert( successful_replace );
    }

    bm.insert( bm_type::value_type(2,"two") );

    // Fail to replace (1,"1") with (1,"two") using the left map view
    {
        assert( bm.size() == 2 );

        bm_type::left_iterator it = bm.left.find(1);

        bool successful_replace = bm.left.replace_data( it, "two" );

        /*<< `it` is still valid here, and the bimap was left unchanged >>*/
        assert( ! successful_replace );
        assert( bm.size() == 2 );
    }
    //]
}

void test_modify()
{
    //[ code_tutorial_modify

    typedef bimap< int, std::string > bm_type;
    bm_type bm;
    bm.insert( bm_type::value_type(1,"one") );

    // Modify (1,"one") to (1,"1") using the right map view
    {
        bm_type::right_iterator it = bm.right.find("one");

        bool successful_modify = bm.right.modify_key( it , _key = "1" );

        assert( successful_modify );
    }

    bm.insert( bm_type::value_type(2,"two") );

    // Fail to modify (1,"1") to (1,"two") using the left map view
    {
        assert( bm.size() == 2 );

        bm_type::left_iterator it = bm.left.find(1);

        bool successful_modify = bm.left.modify_data( it, _data = "two" );

        /*<< `it` is not longer valid and `(1,"1")` is removed from the bimap >>*/
        assert( ! successful_modify );
        assert( bm.size() == 1 );
    }
    //]

    /*
    // Modify (2,"two") to (3,"two") using the set of relations view
    {
        bm_type::iterator it = bm.begin();

        bool successful_modify = bm.modify( it, ++_left );

        assert( successful_modify );
    }
    */
}

int main()
{
    test_replace();
    test_modify();
    return 0;
}


