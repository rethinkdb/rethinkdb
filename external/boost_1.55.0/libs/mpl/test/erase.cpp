
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License,Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: erase.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/erase.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/list_c.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/integral_c.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/aux_/test.hpp>

MPL_TEST_CASE()
{
    typedef list<int,char,long,short,char,long,double,long> types;
    typedef find<types,short>::type iter;
    
    typedef erase<types, iter>::type result;
    MPL_ASSERT_RELATION( size<result>::value, ==, 7 );

    typedef find<result,short>::type result_iter;
    MPL_ASSERT(( is_same< result_iter, end<result>::type > ));
}

MPL_TEST_CASE()
{
    typedef list_c<int,1,0,5,1,7,5,0,5> values;
    typedef find< values, integral_c<int,7> >::type iter;

    typedef erase<values, iter>::type result;
    MPL_ASSERT_RELATION( size<result>::value, ==, 7 );

    typedef find<result, integral_c<int,7> >::type result_iter;
    MPL_ASSERT(( is_same< result_iter, end<result>::type > ));
}
