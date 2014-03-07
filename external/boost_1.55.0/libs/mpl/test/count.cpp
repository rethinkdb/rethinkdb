
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: count.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/count.hpp>

#include <boost/mpl/vector.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/integral_c.hpp>
#include <boost/mpl/aux_/test.hpp>

MPL_TEST_CASE()
{
    typedef vector<int,char,long,short,char,long,double,long> types;

    MPL_ASSERT_RELATION( (count<types,int>::value), ==, 1 );
    MPL_ASSERT_RELATION( (count<types,double>::value), ==, 1 );
    MPL_ASSERT_RELATION( (count<types,char>::value), ==, 2 );
    MPL_ASSERT_RELATION( (count<types,long>::value), ==, 3 );
    MPL_ASSERT_RELATION( (count<types,unsigned>::value), ==, 0 );
}
    
MPL_TEST_CASE()
{
    typedef vector_c<int,1,0,5,1,7,5,0,5> values;

    MPL_ASSERT_RELATION( (count< values, integral_c<int,1> >::value), ==, 2 );
    MPL_ASSERT_RELATION( (count< values, integral_c<int,0> >::value), ==, 2 );
    MPL_ASSERT_RELATION( (count< values, integral_c<int,5> >::value), ==, 3 );
    MPL_ASSERT_RELATION( (count< values, integral_c<int,7> >::value), ==, 1 );
    MPL_ASSERT_RELATION( (count< values, integral_c<int,8> >::value), ==, 0 );
}
