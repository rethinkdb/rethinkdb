
// Copyright Aleksey Gurtovoy 2001-2004
//
// Distributed under the Boost Software License,Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: sizeof.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/sizeof.hpp>
#include <boost/mpl/aux_/test.hpp>

struct my
{
    char a[100];
};

MPL_TEST_CASE()
{
    MPL_ASSERT_RELATION( sizeof_<char>::value, ==, sizeof(char) );
    MPL_ASSERT_RELATION( sizeof_<int>::value, ==, sizeof(int) );
    MPL_ASSERT_RELATION( sizeof_<double>::value, ==, sizeof(double) );
    MPL_ASSERT_RELATION( sizeof_<my>::value, ==, sizeof(my) );
}
