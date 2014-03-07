
// Copyright Aleksey Gurtovoy 2001-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: comparison.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/comparison.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/aux_/test.hpp>

// make sure MSVC behaves nicely in presence of the following template
template< typename T > struct value {};

typedef int_<0> _0;
typedef int_<10> _10;

MPL_TEST_CASE()
{
    MPL_ASSERT_NOT(( equal_to<_0, _10> ));
    MPL_ASSERT_NOT(( equal_to<_10, _0> ));
    MPL_ASSERT(( equal_to<_10, _10> ));
}

MPL_TEST_CASE()
{
    MPL_ASSERT(( not_equal_to<_0, _10> ));
    MPL_ASSERT(( not_equal_to<_10, _0> ));
    MPL_ASSERT_NOT(( not_equal_to<_10, _10> ));
}

MPL_TEST_CASE()
{
    MPL_ASSERT(( less<_0, _10>  ));
    MPL_ASSERT_NOT(( less<_10, _0> ));
    MPL_ASSERT_NOT(( less<_10, _10> ));
}

MPL_TEST_CASE()
{
    MPL_ASSERT(( less_equal<_0, _10> ));
    MPL_ASSERT_NOT(( less_equal<_10, _0> ));
    MPL_ASSERT(( less_equal<_10, _10> ));
}

MPL_TEST_CASE()
{
    MPL_ASSERT(( greater<_10, _0> ));
    MPL_ASSERT_NOT(( greater<_0, _10> ));
    MPL_ASSERT_NOT(( greater<_10, _10> ));
}

MPL_TEST_CASE()
{
    MPL_ASSERT_NOT(( greater_equal<_0, _10> ));
    MPL_ASSERT(( greater_equal<_10, _0> ));
    MPL_ASSERT(( greater_equal<_10, _10> ));
}
