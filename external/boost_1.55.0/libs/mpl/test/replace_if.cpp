
// Copyright Aleksey Gurtovoy 2000-2004
// Copyright John R. Bandela 2000-2002
// Copyright David Abrahams 2003-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: replace_if.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/replace_if.hpp>

#include <boost/mpl/list/list10_c.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/mpl/greater.hpp>
#include <boost/mpl/equal_to.hpp>

#include <boost/mpl/aux_/test.hpp>

MPL_TEST_CASE()
{
    typedef list8_c<int,1,4,5,2,7,5,3,5>::type numbers;
    typedef replace_if< numbers, greater<_1,int_<4> >, int_<0> >::type result;

    typedef list8_c<int,1,4,0,2,0,0,3,0>::type answer;
    MPL_ASSERT(( equal< answer,result,equal_to<_1,_2> > ));
}
