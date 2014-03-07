
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License,Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: find.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/find.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/list_c.hpp>
#include <boost/mpl/distance.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/aux_/test.hpp>

MPL_TEST_CASE()
{    
    typedef list<int,char,long,short,char,long,double,long>::type types;
    typedef list_c<int,1,0,5,1,7,5,0,5> values;

    typedef find<types, short>::type types_iter;
    typedef find< values, integral_c<int,7> >::type values_iter;
   
    MPL_ASSERT(( is_same< deref<types_iter>::type, short> ));
    MPL_ASSERT_RELATION( deref<values_iter>::type::value, ==, 7 );

    typedef begin<types>::type types_first;
    typedef begin<values>::type values_first;
    MPL_ASSERT_RELATION( (mpl::distance< types_first,types_iter >::value), ==, 3 );
    MPL_ASSERT_RELATION( (mpl::distance< values_first,values_iter >::value), ==, 4 );
}
