
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: count_if.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/count_if.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/comparison.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/aux_/test.hpp>

#include <boost/type_traits/is_float.hpp>
#include <boost/type_traits/is_same.hpp>

MPL_TEST_CASE()
{
    typedef vector<int,char&,long,short,char&,long,double,long> types;
    typedef vector_c<int,1,0,5,1,7,5,0,5> values;
    
    MPL_ASSERT_RELATION( (count_if< types, boost::is_float<_> >::value), ==, 1 );
    MPL_ASSERT_RELATION( (count_if< types, boost::is_same<_,char&> >::value), ==, 2 );
    MPL_ASSERT_RELATION( (count_if< types, boost::is_same<_,void*> >::value), ==, 0 );

    MPL_ASSERT_RELATION( (count_if< values, less<_,int_<5> > >::value), ==, 4 );
    MPL_ASSERT_RELATION( (count_if< values, equal_to<int_<0>,_>  >::value), ==, 2 );
    MPL_ASSERT_RELATION( (count_if< values, equal_to<int_<-1>,_> >::value), ==, 0 );
}
