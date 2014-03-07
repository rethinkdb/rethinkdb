
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: at.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/at.hpp>
#include <boost/mpl/vector/vector10_c.hpp>
#include <boost/mpl/aux_/test.hpp>

template< typename Seq, int n > struct at_test
{
    typedef typename at_c<Seq,n>::type t;
    MPL_ASSERT(( is_same< t, integral_c<int,9-n> > ));
    MPL_ASSERT_RELATION( t::value, ==, 9 - n );
};

MPL_TEST_CASE()
{
    typedef vector10_c<int,9,8,7,6,5,4,3,2,1,0> numbers;
    
    at_test< numbers, 0 >();
    at_test< numbers, 1 >();
    at_test< numbers, 2 >();
    at_test< numbers, 3 >();
    at_test< numbers, 4 >();
    at_test< numbers, 5 >();
    at_test< numbers, 6 >();
    at_test< numbers, 7 >();
    at_test< numbers, 8 >();
    at_test< numbers, 9 >();
}
