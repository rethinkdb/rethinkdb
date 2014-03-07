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
#include <cassert>

#include <boost/bimap/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/bimap/list_of.hpp>
#include <boost/bimap/multiset_of.hpp>

using namespace boost::bimaps;

void first_bimap()
{
   //[ code_at_function_first

    typedef bimap< set_of< std::string >, list_of< int > > bm_type;
    bm_type bm;

    try
    {
        bm.left.at("one") = 1; // throws std::out_of_range
    }
    catch( std::out_of_range & e ) {}

    assert( bm.empty() );

    bm.left["one"] = 1; // Ok

    assert( bm.left.at("one") == 1 ); // Ok
    //]
}

void second_bimap()
{
    //[ code_at_function_second

    typedef bimap< multiset_of<std::string>, unordered_set_of<int> > bm_type;
    bm_type bm;

    //<-
    /*
    //->
    bm.right[1] = "one"; // compilation error
    //<-
    */
    //->

    bm.right.insert( bm_type::right_value_type(1,"one") );

    assert( bm.right.at(1) == "one" ); // Ok

    try
    {
        std::cout << bm.right.at(2); // throws std::out_of_range
    }
    catch( std::out_of_range & e ) {}

    //<-
    /*
    //->
    bm.right.at(1) = "1"; // compilation error
    //<-
    */
    //->

    //]
}

int main()
{
    first_bimap();
    second_bimap();
    return 0;
}

