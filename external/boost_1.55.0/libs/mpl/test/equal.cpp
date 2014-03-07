
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: equal.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/equal.hpp>

#include <boost/mpl/list.hpp>
#include <boost/mpl/aux_/test.hpp>

MPL_TEST_CASE()
{
    typedef list<int,float,long,double,char,long,double,float> list1;
    typedef list<int,float,long,double,char,long,double,float> list2;
    typedef list<int,float,long,double,char,long,double,short> list3;
    typedef list<int,float,long,double,char,long,double> list4;
    
    MPL_ASSERT(( equal<list1,list2> ));
    MPL_ASSERT(( equal<list2,list1> ));
    MPL_ASSERT_NOT(( equal<list2,list3> ));
    MPL_ASSERT_NOT(( equal<list3,list4> ));
    MPL_ASSERT_NOT(( equal<list4,list3> ));
}
