
// Copyright Aleksey Gurtovoy 2001-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: arithmetic.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/arithmetic.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/aux_/test.hpp>

MPL_TEST_CASE()
{
    typedef int_<0> _0;
    typedef int_<1> _1;
    typedef int_<3> _3;
    typedef int_<10> _10;

    MPL_ASSERT_RELATION( (plus<_0,_10>::value), ==, 10 );
    MPL_ASSERT_RELATION( (plus<_10,_0>::value), ==, 10 );

    MPL_ASSERT_RELATION( (minus<_0,_10>::value), ==, -10 );
    MPL_ASSERT_RELATION( (minus<_10,_0>::value), ==, 10 );

    MPL_ASSERT_RELATION( (times<_1,_10>::value), ==, 10 );
    MPL_ASSERT_RELATION( (times<_10,_1>::value), ==, 10 );
    MPL_ASSERT_RELATION( (multiplies<_1,_10>::value), ==, 10 );
    MPL_ASSERT_RELATION( (multiplies<_10,_1>::value), ==, 10 );

    MPL_ASSERT_RELATION( (divides<_10,_1>::value), ==, 10 );
    MPL_ASSERT_RELATION( (divides<_10,_1>::value), ==, 10 );

    MPL_ASSERT_RELATION( (modulus<_10,_1>::value), ==, 0 );
    MPL_ASSERT_RELATION( (modulus<_10,_3>::value), ==, 1 );

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
    MPL_ASSERT_RELATION( (minus<_10,_1,_10>::value), ==, -1 );
    MPL_ASSERT_RELATION( (plus<_10,_1,_10>::value), ==, 21 );
    MPL_ASSERT_RELATION( (divides<_10,_1,_10>::value), ==, 1 );
    MPL_ASSERT_RELATION( (divides<_10,_1,_10>::value), ==, 1 );
#endif
    
    MPL_ASSERT_RELATION( negate<_10>::value, ==, -10 );
}
