
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: deque.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/deque.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/mpl/pop_back.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/back.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/empty.hpp>

#include <boost/mpl/aux_/test.hpp>


MPL_TEST_CASE()
{
    typedef deque<> d0;
    typedef deque<char> d1;
    typedef deque<char,long> d2;
    typedef deque<char,char,char,char,char,char,char,char,int> d9;

    MPL_ASSERT_RELATION( size<d0>::value, ==, 0 );
    MPL_ASSERT_RELATION( size<d1>::value, ==, 1 );
    MPL_ASSERT_RELATION( size<d2>::value, ==, 2 );
    MPL_ASSERT_RELATION( size<d9>::value, ==, 9 );

    MPL_ASSERT(( empty<d0> ));
    MPL_ASSERT_NOT(( empty<d1> ));
    MPL_ASSERT_NOT(( empty<d2> ));
    MPL_ASSERT_NOT(( empty<d9> ));

    MPL_ASSERT(( is_same< front<d1>::type,char > ));
    MPL_ASSERT(( is_same< back<d1>::type,char > ));
    MPL_ASSERT(( is_same< front<d2>::type,char > ));
    MPL_ASSERT(( is_same< back<d2>::type,long > ));
    MPL_ASSERT(( is_same< front<d9>::type,char > ));
    MPL_ASSERT(( is_same< back<d9>::type,int > ));
}


MPL_TEST_CASE()
{
    typedef deque<char,long> d2;
    
    typedef begin<d2>::type i1;
    typedef next<i1>::type  i2;
    typedef next<i2>::type  i3;
    
    MPL_ASSERT(( is_same<deref<i1>::type,char> ));
    MPL_ASSERT(( is_same<deref<i2>::type,long> ));
    MPL_ASSERT(( is_same< i3, end<d2>::type > ));
}

MPL_TEST_CASE()
{
    typedef deque<> d0;

    typedef push_back<d0,int>::type d1;
    MPL_ASSERT(( is_same< back<d1>::type,int > ));

    typedef push_front<d1,char>::type d2;
    MPL_ASSERT(( is_same< back<d2>::type,int > ));
    MPL_ASSERT(( is_same< front<d2>::type,char > ));

    typedef push_back<d2,long>::type d3;
    MPL_ASSERT(( is_same< back<d3>::type,long > ));
}

MPL_TEST_CASE()
{
    typedef deque<> d0;
    typedef deque<char> d1;
    typedef deque<char,long> d2;
    typedef deque<char,char,char,char,char,char,char,char,int> d9;

    MPL_ASSERT_RELATION( size<d0>::value, ==, 0 );
    MPL_ASSERT_RELATION( size<d1>::value, ==, 1 );
    MPL_ASSERT_RELATION( size<d2>::value, ==, 2 );
    MPL_ASSERT_RELATION( size<d9>::value, ==, 9 );
}
