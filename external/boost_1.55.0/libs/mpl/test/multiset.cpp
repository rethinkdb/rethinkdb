
// Copyright Aleksey Gurtovoy 2003-2006
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: multiset.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/multiset/multiset0.hpp>
//#include <boost/mpl/multiset/multiset10.hpp>

#include <boost/mpl/insert.hpp>
#include <boost/mpl/count.hpp>
#include <boost/mpl/aux_/test.hpp>

#include <boost/mpl/assert.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/find.hpp>

#include <boost/config.hpp>

/*
struct test_data1
{
    typedef multiset0<>                         s0;
    typedef multiset1<int>                      s1;
    typedef multiset2<int,char&>                s2;
    typedef multiset3<int,char&,int>            s3;
    typedef multiset4<int,char&,int,abstract>   s4;
};

struct test_data2
{
    typedef multiset<>                          s0;
    typedef multiset<int>                       s1;
    typedef multiset<int,char&>                 s2;
    typedef multiset<int,char&,int>             s3;
    typedef multiset<int,char&,int,abstract>    s4;
};
*/

template< typename S0 >
struct test_data
{
    typedef S0                                  s0;
    typedef typename insert<s0,int>::type       s1;
    typedef typename insert<s1,char&>::type     s2;
    typedef typename insert<s2,int>::type       s3;
    typedef typename insert<s3,abstract>::type  s4;
};


template< typename T >
void count_test()
{
    MPL_ASSERT_RELATION( ( count<BOOST_DEDUCED_TYPENAME T::s0,int>::value ), ==, 0 );
    MPL_ASSERT_RELATION( ( count<BOOST_DEDUCED_TYPENAME T::s1,int>::value ), ==, 1 );
    MPL_ASSERT_RELATION( ( count<BOOST_DEDUCED_TYPENAME T::s2,int>::value ), ==, 1 );
    MPL_ASSERT_RELATION( ( count<BOOST_DEDUCED_TYPENAME T::s2,char&>::value ), ==, 1 );
    MPL_ASSERT_RELATION( ( count<BOOST_DEDUCED_TYPENAME T::s3,int>::value ), ==, 2 );
    MPL_ASSERT_RELATION( ( count<BOOST_DEDUCED_TYPENAME T::s3,char&>::value ), ==, 1 );
    MPL_ASSERT_RELATION( ( count<BOOST_DEDUCED_TYPENAME T::s4,abstract>::value ), ==, 1 );
}

MPL_TEST_CASE()
{
    //count_test<test_data1>();
    //count_test<test_data2>();
    //count_test< test_data< multiset<> > >();
    count_test< test_data< multiset0<> > >();
}

/*
// Use a template for testing so that GCC will show us the actual types involved
template <class S>
void find_test()
{
    BOOST_MPL_ASSERT_RELATION( size<S>::value, ==, 3 );

    typedef typename end<S>::type not_found;
    BOOST_MPL_ASSERT_NOT(( is_same<BOOST_DEDUCED_TYPENAME find<S,int>::type,not_found> ));
    BOOST_MPL_ASSERT_NOT(( is_same<BOOST_DEDUCED_TYPENAME find<S,long>::type,not_found> ));
    BOOST_MPL_ASSERT_NOT(( is_same<BOOST_DEDUCED_TYPENAME find<S,char>::type,not_found> ));
    BOOST_MPL_ASSERT(( is_same<BOOST_DEDUCED_TYPENAME find<S,char*>::type,not_found> ));
}
*/

MPL_TEST_CASE()
{
    // agurt 11/jun/06: multiset does not implement iterators yet!    
    // typedef insert<multiset0<>, int>::type set_of_1_int;
    // typedef begin<set_of_1_int>::type iter_to_1_int;
    // BOOST_MPL_ASSERT(( is_same< deref<iter_to_1_int>::type, int > ));
    
    typedef multiset0<> s0;
    typedef insert<s0,int>::type s1;
    typedef insert<s1,long>::type s2;
    typedef insert<s2,char>::type myset;
    
    // find_test<myset>();
    // find_test<myset::type>();
}
