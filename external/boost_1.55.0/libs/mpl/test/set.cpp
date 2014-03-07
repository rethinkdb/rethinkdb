
// Copyright Aleksey Gurtovoy 2003-2007
// Copyright David Abrahams 2003-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: set.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/set.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/next.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/erase.hpp>
#include <boost/mpl/erase_key.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/clear.hpp>
#include <boost/mpl/has_key.hpp>
#include <boost/mpl/order.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/distance.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/begin_end.hpp>

#include <boost/mpl/aux_/test.hpp>


// Use templates for testing so that GCC will show us the actual types involved

template< typename s >
void empty_set_test()
{
    MPL_ASSERT_RELATION( size<s>::value, ==, 0 );
    MPL_ASSERT(( empty<s> ));
    
    MPL_ASSERT(( is_same< BOOST_DEDUCED_TYPENAME clear<s>::type, set0<> > ));
    MPL_ASSERT(( is_same< BOOST_DEDUCED_TYPENAME at<s,int>::type, void_ > ));
    MPL_ASSERT(( is_same< BOOST_DEDUCED_TYPENAME at<s,char>::type, void_ > ));
    MPL_ASSERT(( is_same< BOOST_DEDUCED_TYPENAME at<s,long>::type, void_ > ));

    MPL_ASSERT_NOT(( has_key<s,int> ));
    MPL_ASSERT_NOT(( has_key<s,char> ));
    MPL_ASSERT_NOT(( has_key<s,long> ));

    typedef BOOST_DEDUCED_TYPENAME order<s,int>::type o1;
    typedef BOOST_DEDUCED_TYPENAME order<s,char>::type o2;
    typedef BOOST_DEDUCED_TYPENAME order<s,long>::type o3;
    MPL_ASSERT(( is_same< o1, void_ > ));
    MPL_ASSERT(( is_same< o2, void_ > ));
    MPL_ASSERT(( is_same< o3, void_ > ));
    
    typedef BOOST_DEDUCED_TYPENAME begin<s>::type first;
    typedef BOOST_DEDUCED_TYPENAME end<s>::type last;

    MPL_ASSERT(( is_same<first, last> ));
    MPL_ASSERT_RELATION( (distance<first, last>::value), ==, 0 );
}


template< typename s >
void int_set_test()
{
    MPL_ASSERT_RELATION( size<s>::value, ==, 1 );
    MPL_ASSERT_NOT(( empty<s> ));
    
    MPL_ASSERT(( is_same< BOOST_DEDUCED_TYPENAME clear<s>::type, set0<> > ));
    MPL_ASSERT(( is_same< BOOST_DEDUCED_TYPENAME at<s,int>::type, int > ));
    MPL_ASSERT(( is_same< BOOST_DEDUCED_TYPENAME at<s,char>::type, void_ > ));
    MPL_ASSERT(( is_same< BOOST_DEDUCED_TYPENAME at<s,long>::type, void_ > ));

    MPL_ASSERT(( has_key<s,int> ));
    MPL_ASSERT_NOT(( has_key<s,char> ));
    MPL_ASSERT_NOT(( has_key<s,long> ));

    typedef BOOST_DEDUCED_TYPENAME order<s,int>::type o1;
    typedef BOOST_DEDUCED_TYPENAME order<s,char>::type o2;
    typedef BOOST_DEDUCED_TYPENAME order<s,long>::type o3;
    MPL_ASSERT_NOT(( is_same< o1, void_ > ));
    MPL_ASSERT(( is_same< o2, void_ > ));
    MPL_ASSERT(( is_same< o3, void_ > ));
    
    typedef BOOST_DEDUCED_TYPENAME begin<s>::type first;
    typedef BOOST_DEDUCED_TYPENAME end<s>::type last;

    MPL_ASSERT(( is_same< BOOST_DEDUCED_TYPENAME deref<first>::type, int > ));
    MPL_ASSERT(( is_same< BOOST_DEDUCED_TYPENAME next<first>::type, last > ));

    MPL_ASSERT_RELATION( (distance<first, last>::value), ==, 1 );
    MPL_ASSERT(( contains< s, int > ));
}


template< typename s >
void int_char_set_test()
{
    MPL_ASSERT_RELATION( size<s>::value, ==, 2 );
    MPL_ASSERT_NOT(( empty<s> ));
    MPL_ASSERT(( is_same< BOOST_DEDUCED_TYPENAME clear<s>::type, set0<> > ));
    MPL_ASSERT(( is_same< BOOST_DEDUCED_TYPENAME at<s,int>::type, int > ));
    MPL_ASSERT(( is_same< BOOST_DEDUCED_TYPENAME at<s,char>::type, char > ));

    MPL_ASSERT(( has_key<s,char> ));
    MPL_ASSERT_NOT(( has_key<s,long> ));

    typedef BOOST_DEDUCED_TYPENAME order<s,int>::type o1;
    typedef BOOST_DEDUCED_TYPENAME order<s,char>::type o2;
    typedef BOOST_DEDUCED_TYPENAME order<s,long>::type o3;
    MPL_ASSERT_NOT(( is_same< o1, void_ > ));
    MPL_ASSERT_NOT(( is_same< o2, void_ > ));
    MPL_ASSERT(( is_same< o3, void_ > ));
    MPL_ASSERT_NOT(( is_same< o1, o2 > ));

    typedef BOOST_DEDUCED_TYPENAME begin<s>::type first;
    typedef BOOST_DEDUCED_TYPENAME end<s>::type last;

    MPL_ASSERT_RELATION( (distance<first, last>::value), ==, 2 );

    MPL_ASSERT(( contains< s, int > ));
    MPL_ASSERT(( contains< s, char > ));
}

template< typename s >
void int_char_long_set_test()
{
    MPL_ASSERT_RELATION( size<s>::value, ==, 3 );
    MPL_ASSERT_NOT(( empty<s> ));
    MPL_ASSERT(( is_same< BOOST_DEDUCED_TYPENAME clear<s>::type, set0<> > ));
    MPL_ASSERT(( is_same< BOOST_DEDUCED_TYPENAME at<s,int>::type, int > ));
    MPL_ASSERT(( is_same< BOOST_DEDUCED_TYPENAME at<s,char>::type, char > ));
    MPL_ASSERT(( is_same< BOOST_DEDUCED_TYPENAME at<s,long>::type, long > ));

    MPL_ASSERT(( has_key<s,long> ));
    MPL_ASSERT(( has_key<s,int> ));
    MPL_ASSERT(( has_key<s,char> ));

    typedef BOOST_DEDUCED_TYPENAME order<s,int>::type o1;
    typedef BOOST_DEDUCED_TYPENAME order<s,char>::type o2;
    typedef BOOST_DEDUCED_TYPENAME order<s,long>::type o3;
    MPL_ASSERT_NOT(( is_same< o1, void_ > ));
    MPL_ASSERT_NOT(( is_same< o2, void_ > ));
    MPL_ASSERT_NOT(( is_same< o3, void_ > ));
    MPL_ASSERT_NOT(( is_same< o1, o2 > ));
    MPL_ASSERT_NOT(( is_same< o1, o3 > ));
    MPL_ASSERT_NOT(( is_same< o2, o3 > ));

    typedef BOOST_DEDUCED_TYPENAME begin<s>::type first;
    typedef BOOST_DEDUCED_TYPENAME end<s>::type last;
    MPL_ASSERT_RELATION( (distance<first, last>::value), ==, 3 );

    MPL_ASSERT(( contains< s, int > ));
    MPL_ASSERT(( contains< s, char > ));
    MPL_ASSERT(( contains< s, long > ));
}

template< typename S0, typename S1, typename S2, typename S3 >
void basic_set_test()
{
    empty_set_test<S0>();
    empty_set_test< BOOST_DEDUCED_TYPENAME erase_key<S1,int>::type  >();
    empty_set_test< BOOST_DEDUCED_TYPENAME erase_key< 
          BOOST_DEDUCED_TYPENAME erase_key<S2,char>::type
        , int
        >::type >();

    empty_set_test< BOOST_DEDUCED_TYPENAME erase_key<
          BOOST_DEDUCED_TYPENAME erase_key<
               BOOST_DEDUCED_TYPENAME erase_key<S3,char>::type
              , long
              >::type
        , int
        >::type >();


    int_set_test<S1>();
    int_set_test< BOOST_DEDUCED_TYPENAME insert<S0,int>::type >();

    int_set_test< BOOST_DEDUCED_TYPENAME erase_key<S2,char>::type >();
    int_set_test< BOOST_DEDUCED_TYPENAME erase_key< 
          BOOST_DEDUCED_TYPENAME erase_key<S3,char>::type
        , long
        >::type >();    

    int_char_set_test<S2>();
    int_char_set_test< BOOST_DEDUCED_TYPENAME insert<
          BOOST_DEDUCED_TYPENAME insert<S0,char>::type  
        , int
        >::type >();

    int_char_set_test< BOOST_DEDUCED_TYPENAME insert<S1,char>::type  >();
    int_char_set_test< BOOST_DEDUCED_TYPENAME erase_key<S3,long>::type >();

    int_char_long_set_test<S3>();
    int_char_long_set_test< BOOST_DEDUCED_TYPENAME insert<
          BOOST_DEDUCED_TYPENAME insert<
               BOOST_DEDUCED_TYPENAME insert<S0,char>::type
              , long
              >::type
        , int
        >::type >();

    int_char_long_set_test< BOOST_DEDUCED_TYPENAME insert<          
          BOOST_DEDUCED_TYPENAME insert<S1,long>::type
        , char
        >::type >();

    int_char_long_set_test< BOOST_DEDUCED_TYPENAME insert<S2,long>::type  >();
}


template< typename S1, typename S2 >
void numbered_vs_variadic_set_test()
{
    MPL_ASSERT(( is_same< S1, BOOST_DEDUCED_TYPENAME S1::type > ));
    MPL_ASSERT(( is_same< BOOST_DEDUCED_TYPENAME S2::type, S1 > ));
}


MPL_TEST_CASE()
{
    typedef mpl::set0<> s01;
    typedef mpl::set<>  s02;
    typedef mpl::set1<int> s11;
    typedef mpl::set<int>  s12;
    typedef mpl::set2<int,char> s21;
    typedef mpl::set<int,char>  s22;
    typedef mpl::set<char,int>  s23;
    typedef mpl::set3<int,char,long> s31;
    typedef mpl::set<int,char,long>  s32;
    typedef mpl::set<int,long,char>  s33;
    typedef mpl::set<long,char,int>  s34;

    numbered_vs_variadic_set_test<s01,s02>();
    numbered_vs_variadic_set_test<s11,s12>();
    numbered_vs_variadic_set_test<s21,s22>();
    numbered_vs_variadic_set_test<s31,s32>();

    basic_set_test<s01,s11,s21,s31>();
    basic_set_test<s02,s12,s22,s32>();
    basic_set_test<s01,s11,s23,s31>();
    basic_set_test<s01,s11,s23,s33>();
    basic_set_test<s01,s11,s23,s34>();
}


template< typename s >
void empty_set_types_variety_test()
{
    MPL_ASSERT_NOT(( has_key<s,char> ));
    MPL_ASSERT_NOT(( has_key<s,int> ));
    MPL_ASSERT_NOT(( has_key<s,UDT> ));
    MPL_ASSERT_NOT(( has_key<s,incomplete> ));

    MPL_ASSERT_NOT(( has_key<s,char const> ));
    MPL_ASSERT_NOT(( has_key<s,int const> ));
    MPL_ASSERT_NOT(( has_key<s,UDT const> ));
    MPL_ASSERT_NOT(( has_key<s,incomplete const> ));

    MPL_ASSERT_NOT(( has_key<s,int*> ));
    MPL_ASSERT_NOT(( has_key<s,UDT*> ));
    MPL_ASSERT_NOT(( has_key<s,incomplete*> ));

    MPL_ASSERT_NOT(( has_key<s,int&> ));
    MPL_ASSERT_NOT(( has_key<s,UDT&> ));
    MPL_ASSERT_NOT(( has_key<s,incomplete&> ));
}

template< typename s >
void set_types_variety_test()
{
    MPL_ASSERT_RELATION( size<s>::value, ==, 8 );

    MPL_ASSERT(( has_key<s,char> ));
    MPL_ASSERT(( has_key<s,int const> ));
    MPL_ASSERT(( has_key<s,long*> ));
    MPL_ASSERT(( has_key<s,UDT* const> ));
    MPL_ASSERT(( has_key<s,incomplete> ));
    MPL_ASSERT(( has_key<s,abstract> ));
    MPL_ASSERT(( has_key<s,incomplete volatile&> ));
    MPL_ASSERT(( has_key<s,abstract const&> ));

    MPL_ASSERT_NOT(( has_key<s,char const> ));
    MPL_ASSERT_NOT(( has_key<s,int> ));
    MPL_ASSERT_NOT(( has_key<s,long* const> ));
    MPL_ASSERT_NOT(( has_key<s,UDT*> ));
    MPL_ASSERT_NOT(( has_key<s,incomplete const> ));
    MPL_ASSERT_NOT(( has_key<s,abstract volatile> ));
    MPL_ASSERT_NOT(( has_key<s,incomplete&> ));
    MPL_ASSERT_NOT(( has_key<s,abstract&> ));
}


MPL_TEST_CASE()
{
    empty_set_types_variety_test< set<> >();
    empty_set_types_variety_test< set<>::type >();

    typedef set<
          char,int const,long*,UDT* const,incomplete,abstract
        , incomplete volatile&,abstract const&
        > s;

    set_types_variety_test<s>();
    set_types_variety_test<s::type>();
}


template <class S>
void find_test()
{
    MPL_ASSERT_RELATION( size<S>::value, ==, 3 );

    typedef typename end<S>::type not_found;
    BOOST_MPL_ASSERT_NOT(( is_same<BOOST_DEDUCED_TYPENAME find<S,int>::type,not_found> ));
    BOOST_MPL_ASSERT_NOT(( is_same<BOOST_DEDUCED_TYPENAME find<S,long>::type,not_found> ));
    BOOST_MPL_ASSERT_NOT(( is_same<BOOST_DEDUCED_TYPENAME find<S,char>::type,not_found> ));
    BOOST_MPL_ASSERT(( is_same<BOOST_DEDUCED_TYPENAME find<S,char*>::type,not_found> ));
}

MPL_TEST_CASE()
{
    typedef mpl::set<int,long,char> s;
    find_test<s>();
    find_test<s::type>();
}
