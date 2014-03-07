
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: list.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/list.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/empty.hpp>

#include <boost/mpl/aux_/test.hpp>


MPL_TEST_CASE()
{
    typedef list0<> l0;
    typedef list1<char> l1;
    typedef list2<char,long> l2;
    typedef list9<char,char,char,char,char,char,char,char,char> l9;

    MPL_ASSERT_RELATION(size<l0>::value, ==, 0);
    MPL_ASSERT_RELATION(size<l1>::value, ==, 1);
    MPL_ASSERT_RELATION(size<l2>::value, ==, 2);
    MPL_ASSERT_RELATION(size<l9>::value, ==, 9);

    MPL_ASSERT(( empty<l0> ));
    MPL_ASSERT_NOT(( empty<l1> ));
    MPL_ASSERT_NOT(( empty<l2> ));
    MPL_ASSERT_NOT(( empty<l9> ));

    MPL_ASSERT(( is_same<front<l1>::type,char> ));
    MPL_ASSERT(( is_same<front<l2>::type,char> ));
    MPL_ASSERT(( is_same<front<l9>::type,char> ));
}

MPL_TEST_CASE()
{
    typedef list2<char,long> l2;
    
    typedef begin<l2>::type i1;
    typedef next<i1>::type  i2;
    typedef next<i2>::type  i3;
    
    MPL_ASSERT(( is_same<deref<i1>::type,char> ));
    MPL_ASSERT(( is_same<deref<i2>::type,long> ));
    MPL_ASSERT(( is_same< i3, end<l2>::type > ));
}

MPL_TEST_CASE()
{
    typedef list0<> l0;

    typedef push_front<l0,char>::type l1;
    MPL_ASSERT(( is_same<front<l1>::type,char> ));

    typedef push_front<l1,long>::type l2;
    MPL_ASSERT(( is_same<front<l2>::type,long> ));
}
