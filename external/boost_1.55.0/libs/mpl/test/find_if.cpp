
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License,Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: find_if.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/find_if.hpp>

#include <boost/mpl/vector.hpp>
#include <boost/mpl/distance.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/aux_/test.hpp>

#include <boost/type_traits/is_float.hpp>
#include <boost/type_traits/is_same.hpp>

typedef vector<int,char,long,short,char,long,double,float,char>::type types;
typedef begin<types>::type first_;

MPL_TEST_CASE()
{
    typedef find_if< types, boost::is_float<_> >::type iter;
    MPL_ASSERT(( is_same< iter::type, double > ));
    MPL_ASSERT_RELATION( (mpl::distance<first_,iter>::value), ==, 6 );
}

MPL_TEST_CASE()
{
    typedef find_if< types, boost::is_same<_,long> >::type iter;
    MPL_ASSERT(( is_same< iter::type, long > ));
    MPL_ASSERT_RELATION( (mpl::distance<first_,iter>::value), ==, 2 );
}

MPL_TEST_CASE()
{
    typedef find_if< types, boost::is_same<_,void> >::type iter;
    MPL_ASSERT(( is_same< iter, end<types>::type > ));
    MPL_ASSERT_RELATION( (mpl::distance<first_,iter>::value), ==, size<types>::value );
}
