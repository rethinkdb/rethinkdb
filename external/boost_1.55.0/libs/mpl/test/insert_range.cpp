
// Copyright Aleksey Gurtovoy 2001-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: insert_range.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/insert_range.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/equal.hpp>

#include <boost/mpl/aux_/test.hpp>

MPL_TEST_CASE()
{
    typedef vector_c<int,0,1,7,8,9> numbers;
    typedef find< numbers,integral_c<int,7> >::type pos;
    typedef insert_range< numbers,pos,range_c<int,2,7> >::type range;

    MPL_ASSERT_RELATION( size<range>::value, ==, 10 );
    MPL_ASSERT(( equal< range,range_c<int,0,10> > ));

    typedef insert_range< list0<>,end< list0<> >::type,list1<int> >::type result2;
    MPL_ASSERT_RELATION( size<result2>::value, ==, 1 );
}
