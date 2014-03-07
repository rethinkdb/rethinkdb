
// Copyright Eric Friedman 2003
//
// Distributed under the Boost Software License,Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: index_of.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $


#include <boost/mpl/index_of.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/void.hpp>
#include <boost/mpl/aux_/test.hpp>

MPL_TEST_CASE()
{
    typedef list< int, double, float >::type types;

    typedef index_of< types, int    >::type index_of_int;
    typedef index_of< types, double >::type index_of_double;
    typedef index_of< types, float  >::type index_of_float;
    typedef index_of< types, char   >::type index_of_char;

    MPL_ASSERT_RELATION( index_of_int::value, ==, 0 );
    MPL_ASSERT_RELATION( index_of_double::value, ==, 1 );
    MPL_ASSERT_RELATION( index_of_float::value, ==, 2 );

    MPL_ASSERT(( is_void_< index_of_char > ));
}
