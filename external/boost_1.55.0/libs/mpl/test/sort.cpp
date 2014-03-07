
// Copyright Aleksey Gurtovoy 2004
// Copyright Eric Friedman 2002-2003
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

#include <boost/mpl/sort.hpp>

#include <boost/mpl/list_c.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/mpl/equal_to.hpp>

#include <boost/mpl/aux_/test.hpp>

MPL_TEST_CASE()
{
    typedef list_c<int, 3, 4, 0, -5, 8, -1, 7>::type numbers;
    typedef list_c<int, -5, -1, 0, 3, 4, 7, 8>::type manual_result;

    typedef sort< numbers >::type result;

    MPL_ASSERT(( equal< result,manual_result,equal_to<_1,_2> > ));
}
