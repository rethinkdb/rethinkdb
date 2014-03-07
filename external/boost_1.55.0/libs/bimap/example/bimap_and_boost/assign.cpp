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

#include <boost/assign/list_of.hpp>
#include <boost/assign/list_inserter.hpp>

#include <boost/bimap/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/bimap/list_of.hpp>

using namespace boost::bimaps;
using namespace boost;


int main()
{
    //[ code_bimap_and_boost_assign

    typedef bimap< multiset_of< int >, list_of< std::string > > bm_type;

    // We can use assign::list_of to initialize the container.

    bm_type bm = assign::list_of< bm_type::relation > /*<
        Note that `bm_type::relation` has to be used instead of `bm_type::value_type`.
        Contrary to `value_type`, `relation` type stores the elements as non const, a
        requirement of `assign::list_of` >*/
        ( 1, "one"   )
        ( 2, "two"   )
        ( 3, "three" );

    // The left map view is a multiset, again we use insert

    assign::insert( bm.left )
        ( 4, "four" )
        ( 5, "five" )
        ( 6, "six"  );

    // The right map view is a list so we use push_back here
    // Note the order of the elements in the list!

    assign::push_back( bm.right )
        ( "seven" , 7 )
        ( "eight" , 8 );

    assign::push_front( bm.right )
        ( "nine"  ,  9 )
        ( "ten"   , 10 )
        ( "eleven", 11 );

   // Since it is left_based the main view is a multiset, so we use insert

    assign::insert( bm )
        ( 12, "twelve"   )
        ( 13, "thirteen" );
    //]

    return 0;
}
