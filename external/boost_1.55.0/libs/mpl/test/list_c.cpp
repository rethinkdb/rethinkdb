
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: list_c.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/list_c.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/size.hpp>
#include <boost/static_assert.hpp>

#include <boost/mpl/aux_/test.hpp>


#if !BOOST_WORKAROUND(BOOST_MSVC,<= 1200)
MPL_TEST_CASE()
{
    typedef list_c<bool,true>::type l1;
    typedef list_c<bool,false>::type l2;

    MPL_ASSERT(( is_same< l1::value_type, bool > ));
    MPL_ASSERT(( is_same< l2::value_type, bool > ));

    MPL_ASSERT_RELATION( front<l1>::type::value, ==, true );
    MPL_ASSERT_RELATION( front<l2>::type::value, ==, false );
}
#endif

MPL_TEST_CASE()
{
    typedef list_c<int,-1>::type l1;
    typedef list_c<int,0,1>::type l2;
    typedef list_c<int,1,2,3>::type l3;

    MPL_ASSERT(( is_same< l1::value_type, int > ));
    MPL_ASSERT(( is_same< l2::value_type, int > ));
    MPL_ASSERT(( is_same< l3::value_type, int > ));

    MPL_ASSERT_RELATION( size<l1>::value, ==, 1 );
    MPL_ASSERT_RELATION( size<l2>::value, ==, 2 );
    MPL_ASSERT_RELATION( size<l3>::value, ==, 3 );
    MPL_ASSERT_RELATION( front<l1>::type::value, ==, -1 );
    MPL_ASSERT_RELATION( front<l2>::type::value, ==, 0 );
    MPL_ASSERT_RELATION( front<l3>::type::value, ==, 1 );
}

MPL_TEST_CASE()
{
    typedef list_c<unsigned,0>::type l1;
    typedef list_c<unsigned,1,2>::type l2;

    MPL_ASSERT(( is_same< l1::value_type, unsigned > ));
    MPL_ASSERT(( is_same< l2::value_type, unsigned > ));

    MPL_ASSERT_RELATION( size<l1>::value, ==, 1 );
    MPL_ASSERT_RELATION( size<l2>::value, ==, 2 );
    MPL_ASSERT_RELATION( front<l1>::type::value, ==, 0 );
    MPL_ASSERT_RELATION( front<l2>::type::value, ==, 1 );
}

MPL_TEST_CASE()
{
    typedef list_c<unsigned,2,1> l2;

    MPL_ASSERT(( is_same< l2::value_type, unsigned > ));
    
    typedef begin<l2>::type i1;
    typedef next<i1>::type  i2;
    typedef next<i2>::type  i3;
    
    MPL_ASSERT_RELATION( deref<i1>::type::value, ==, 2 );
    MPL_ASSERT_RELATION( deref<i2>::type::value, ==, 1 );
    MPL_ASSERT(( is_same< i3, end<l2>::type > ));
}
