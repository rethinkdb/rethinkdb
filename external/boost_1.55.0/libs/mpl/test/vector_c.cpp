
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: vector_c.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/size.hpp>

#include <boost/mpl/aux_/test.hpp>

#if !BOOST_WORKAROUND(BOOST_MSVC, <=1200)
MPL_TEST_CASE()
{
    typedef vector_c<bool,true>::type v1;
    typedef vector_c<bool,false>::type v2;

    MPL_ASSERT(( is_same< v1::value_type, bool > ));
    MPL_ASSERT(( is_same< v2::value_type, bool > ));

    MPL_ASSERT_RELATION( front<v1>::type::value, ==, true );
    MPL_ASSERT_RELATION( front<v2>::type::value, ==, false );
}
#endif

MPL_TEST_CASE()
{
    typedef vector_c<int,-1> v1;
    typedef vector_c<int,0,1> v2;
    typedef vector_c<int,1,2,3> v3;

    MPL_ASSERT(( is_same< v1::value_type, int > ));
    MPL_ASSERT(( is_same< v2::value_type, int > ));
    MPL_ASSERT(( is_same< v3::value_type, int > ));

    MPL_ASSERT_RELATION( size<v1>::value, ==, 1 );
    MPL_ASSERT_RELATION( size<v2>::value, ==, 2 );
    MPL_ASSERT_RELATION( size<v3>::value, ==, 3 );

    MPL_ASSERT_RELATION( front<v1>::type::value, ==, -1 );
    MPL_ASSERT_RELATION( front<v2>::type::value, ==, 0 );
    MPL_ASSERT_RELATION( front<v3>::type::value, ==, 1 );
}

MPL_TEST_CASE()
{
    typedef vector_c<unsigned,0> v1;
    typedef vector_c<unsigned,1,2> v2;

    MPL_ASSERT(( is_same< v1::value_type, unsigned > ));
    MPL_ASSERT(( is_same< v2::value_type, unsigned > ));

    MPL_ASSERT_RELATION( size<v1>::type::value, ==, 1 );
    MPL_ASSERT_RELATION( size<v2>::type::value, ==, 2 );

    MPL_ASSERT_RELATION( front<v1>::type::value, ==, 0 );
    MPL_ASSERT_RELATION( front<v2>::type::value, ==, 1 );
}
