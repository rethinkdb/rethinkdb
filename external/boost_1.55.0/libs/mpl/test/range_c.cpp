
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License,Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: range_c.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/range_c.hpp>
#include <boost/mpl/advance.hpp>
#include <boost/mpl/distance.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/back.hpp>
#include <boost/mpl/aux_/test.hpp>

MPL_TEST_CASE()
{
    typedef range_c<int,0,0> range0;
    typedef range_c<int,0,1> range1;
    typedef range_c<int,0,10> range10;

    MPL_ASSERT_RELATION( size<range0>::value, ==, 0 );
    MPL_ASSERT_RELATION( size<range1>::value, ==, 1 );
    MPL_ASSERT_RELATION( size<range10>::value, ==, 10 );

    MPL_ASSERT(( empty<range0> ));
    MPL_ASSERT_NOT(( empty<range1> ));
    MPL_ASSERT_NOT(( empty<range10> ));

    MPL_ASSERT(( is_same< begin<range0>::type, end<range0>::type > ));
    MPL_ASSERT_NOT(( is_same<begin<range1>::type, end<range1>::type > ));
    MPL_ASSERT_NOT(( is_same<begin<range10>::type, end<range10>::type > ));

    MPL_ASSERT_RELATION( front<range1>::type::value, ==, 0 );
    MPL_ASSERT_RELATION( back<range1>::type::value, ==, 0 );
    MPL_ASSERT_RELATION( front<range10>::type::value, ==, 0 );
    MPL_ASSERT_RELATION( back<range10>::type::value, ==, 9 );
}

MPL_TEST_CASE()
{
    typedef range_c<unsigned char,0,10> r;
    typedef begin<r>::type first;
    typedef end<r>::type last;

    MPL_ASSERT(( is_same< advance_c<first,10>::type, last > ));
    MPL_ASSERT(( is_same< advance_c<last,-10>::type, first > ));

    MPL_ASSERT_RELATION( ( mpl::distance<first,last>::value ), ==, 10 );

    typedef advance_c<first,5>::type iter;
    MPL_ASSERT_RELATION( deref<iter>::type::value, ==, 5 );
}
