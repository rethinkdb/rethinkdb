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

#include <iostream>
#include <cassert>

// A convinience header is avaiable in the boost directory:
#include <boost/bimap.hpp>

int main()
{
    //[ code_step_by_step_definition

    typedef boost::bimap< int, std::string > bm_type;
    bm_type bm;
    //]

    //[ code_step_by_step_set_of_relations_view

    bm.insert( bm_type::value_type(1, "one" ) );
    bm.insert( bm_type::value_type(2, "two" ) );

    std::cout << "There are " << bm.size() << "relations" << std::endl;

    for( bm_type::const_iterator iter = bm.begin(), iend = bm.end(); 
        iter != iend; ++iter )
    {
        // iter->left  : data : int
        // iter->right : data : std::string

        std::cout << iter->left << " <--> " << iter->right << std::endl;
    }
    //]

    //[ code_step_by_step_left_map_view

    /*<< The type of `bm.left` is `bm_type::left_map` and the type
         of `bm.right` is `bm_type::right_map` >>*/
    typedef bm_type::left_map::const_iterator left_const_iterator;

    for( left_const_iterator left_iter = bm.left.begin(), iend = bm.left.end();
         left_iter != iend; ++left_iter )
    {
        // left_iter->first  : key  : int
        // left_iter->second : data : std::string

        std::cout << left_iter->first << " --> " << left_iter->second << std::endl;
    }

    /*<< `bm_type::left_`\ -type- can be used as a shortcut for the more verbose
    `bm_type::left_map::`\ -type- >>*/ 
    bm_type::left_const_iterator left_iter = bm.left.find(2);
    assert( left_iter->second ==  "two" );

    /*<< This line produces the same effect of
         `bm.insert( bm_type::value_type(3,"three") );` >>*/
    bm.left.insert( bm_type::left_value_type( 3, "three" ) );
    //]



    //[ code_step_by_step_right_map_view

    bm_type::right_const_iterator right_iter = bm.right.find("two");

    // right_iter->first  : key  : std::string
    // right_iter->second : data : int

    assert( right_iter->second ==  2 );

    assert( bm.right.at("one") == 1 );

    bm.right.erase("two");

    /*<< This line produces the same effect of
         `bm.insert( bm_type::value_type(4,"four") );` >>*/
    bm.right.insert( bm_type::right_value_type( "four", 4 ) );
    //]

    return 0;
}


