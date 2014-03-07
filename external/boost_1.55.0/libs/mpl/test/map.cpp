
// Copyright Aleksey Gurtovoy 2003-2004
// Copyright David Abrahams 2003-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: map.cpp 55752 2009-08-24 04:17:30Z agurtovoy $
// $Date: 2009-08-23 21:17:30 -0700 (Sun, 23 Aug 2009) $
// $Revision: 55752 $

#include <boost/mpl/map.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/erase_key.hpp>
#include <boost/mpl/erase_key.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/clear.hpp>
#include <boost/mpl/has_key.hpp>
#include <boost/mpl/order.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/mpl/aux_/test.hpp>


MPL_TEST_CASE()
{
    typedef map2<
          mpl::pair<int,unsigned>
        , mpl::pair<char,unsigned char>
        > m_;

    typedef erase_key<m_,char>::type m;

    MPL_ASSERT_RELATION( size<m>::type::value, ==, 1 );
    MPL_ASSERT_NOT(( empty<m> ));
    MPL_ASSERT(( is_same< clear<m>::type,map0<> > ));
    
    MPL_ASSERT(( is_same< at<m,int>::type,unsigned > ));
    MPL_ASSERT(( is_same< at<m,char>::type,void_ > ));
    MPL_ASSERT(( contains< m,mpl::pair<int,unsigned> > ));
    MPL_ASSERT_NOT(( contains< m,mpl::pair<int,int> > ));
    MPL_ASSERT_NOT(( contains< m,mpl::pair<char,unsigned char> > ));

    MPL_ASSERT_NOT(( has_key<m,char>::type ));
    MPL_ASSERT(( has_key<m,int>::type ));
    
    MPL_ASSERT_NOT(( is_same< order<m,int>::type, void_ > ));
    MPL_ASSERT(( is_same< order<m,char>::type,void_ > ));

    typedef begin<m>::type first;
    typedef end<m>::type last;

    MPL_ASSERT(( is_same< deref<first>::type,mpl::pair<int,unsigned> > ));
    MPL_ASSERT(( is_same< next<first>::type,last > ));

    typedef insert<m,mpl::pair<char,long> >::type m2;

    MPL_ASSERT_RELATION( size<m2>::type::value, ==, 2 );
    MPL_ASSERT_NOT(( empty<m2>::type ));
    MPL_ASSERT(( is_same< clear<m2>::type,map0<> > ));
    MPL_ASSERT(( is_same< at<m2,int>::type,unsigned > ));
    MPL_ASSERT(( is_same< at<m2,char>::type,long > ));

    MPL_ASSERT(( contains< m2,mpl::pair<int,unsigned> > ));
    MPL_ASSERT_NOT(( contains< m2,mpl::pair<int,int> > ));
    MPL_ASSERT_NOT(( contains< m2,mpl::pair<char,unsigned char> > ));
    MPL_ASSERT(( contains< m2,mpl::pair<char,long> > ));

    MPL_ASSERT(( has_key<m2,char>::type ));
    MPL_ASSERT_NOT(( has_key<m2,long>::type ));
    MPL_ASSERT_NOT(( is_same< order<m2,int>::type, void_ > ));
    MPL_ASSERT_NOT(( is_same< order<m2,char>::type, void_ > ));
    MPL_ASSERT_NOT(( is_same< order<m2,char>::type, order<m2,int>::type > ));

    typedef begin<m2>::type first2;
    typedef end<m2>::type last2;

    MPL_ASSERT(( is_same<deref<first2>::type,mpl::pair<int,unsigned> > ));
    typedef next<first2>::type iter;
    MPL_ASSERT(( is_same<deref<iter>::type,mpl::pair<char,long> > ));
    MPL_ASSERT(( is_same< next<iter>::type,last2 > ));

    typedef insert<m2,mpl::pair<int,unsigned> >::type s2_1;
    MPL_ASSERT(( is_same<m2,s2_1> ));

    typedef insert<m2,mpl::pair<long,unsigned> >::type m3;
    MPL_ASSERT_RELATION( size<m3>::type::value, ==, 3 );
    MPL_ASSERT(( has_key<m3,long>::type ));
    MPL_ASSERT(( has_key<m3,int>::type ));
    MPL_ASSERT(( has_key<m3,char>::type ));
    MPL_ASSERT(( contains< m3,mpl::pair<long,unsigned> > ));
    MPL_ASSERT(( contains< m3,mpl::pair<int,unsigned> > ));

    typedef insert<m,mpl::pair<char,long> >::type m1;
    MPL_ASSERT_RELATION( size<m1>::type::value, ==, 2 );
    MPL_ASSERT(( is_same< at<m1,int>::type,unsigned > ));
    MPL_ASSERT(( is_same< at<m1,char>::type,long > ));

    MPL_ASSERT(( contains< m1,mpl::pair<int,unsigned> > ));
    MPL_ASSERT_NOT(( contains< m1,mpl::pair<int,int> > ));
    MPL_ASSERT_NOT(( contains< m1,mpl::pair<char,unsigned char> > ));
    MPL_ASSERT(( contains< m1,mpl::pair<char,long> > ));

    MPL_ASSERT(( is_same< m1,m2 > ));

    typedef erase_key<m1,char>::type m_1;
    MPL_ASSERT(( is_same<m,m_1> ));
    MPL_ASSERT_RELATION( size<m_1>::type::value, ==, 1 );
    MPL_ASSERT(( is_same< at<m_1,char>::type,void_ > ));
    MPL_ASSERT(( is_same< at<m_1,int>::type,unsigned > ));

    typedef erase_key<m3,char>::type m2_1;
    MPL_ASSERT_RELATION( size<m2_1>::type::value, ==, 2 );
    MPL_ASSERT(( is_same< at<m2_1,char>::type,void_ > ));
    MPL_ASSERT(( is_same< at<m2_1,int>::type,unsigned > ));
    MPL_ASSERT(( is_same< at<m2_1,long>::type,unsigned > ));
}

MPL_TEST_CASE()
{
    typedef map0<> m;
    
    MPL_ASSERT_RELATION( size<m>::type::value, ==, 0 );
    MPL_ASSERT(( empty<m>::type ));

    MPL_ASSERT(( is_same< clear<m>::type,map0<> > ));
    MPL_ASSERT(( is_same< at<m,char>::type,void_ > ));

    MPL_ASSERT_NOT(( has_key<m,char>::type ));
    MPL_ASSERT_NOT(( has_key<m,int>::type ));
    MPL_ASSERT_NOT(( has_key<m,UDT>::type ));
    MPL_ASSERT_NOT(( has_key<m,incomplete>::type ));

    MPL_ASSERT_NOT(( has_key<m,char const>::type ));
    MPL_ASSERT_NOT(( has_key<m,int const>::type ));
    MPL_ASSERT_NOT(( has_key<m,UDT const>::type ));
    MPL_ASSERT_NOT(( has_key<m,incomplete const>::type ));

    MPL_ASSERT_NOT(( has_key<m,int*>::type ));
    MPL_ASSERT_NOT(( has_key<m,UDT*>::type ));
    MPL_ASSERT_NOT(( has_key<m,incomplete*>::type ));

    MPL_ASSERT_NOT(( has_key<m,int&>::type ));
    MPL_ASSERT_NOT(( has_key<m,UDT&>::type ));
    MPL_ASSERT_NOT(( has_key<m,incomplete&>::type ));

    typedef insert<m,mpl::pair<char,int> >::type m1;
    MPL_ASSERT_RELATION( size<m1>::type::value, ==, 1 );
    MPL_ASSERT(( is_same< at<m1,char>::type,int > ));

    typedef erase_key<m,char>::type m0_1;
    MPL_ASSERT_RELATION( size<m0_1>::type::value, ==, 0 );
    MPL_ASSERT(( is_same< at<m0_1,char>::type,void_ > ));
}

// Use a template for testing so that GCC will show us the actual types involved
template <class M>
void test()
{
    MPL_ASSERT_RELATION( size<M>::value, ==, 3 );

    typedef typename end<M>::type not_found;
    BOOST_MPL_ASSERT_NOT(( is_same<BOOST_DEDUCED_TYPENAME find<M,mpl::pair<int,int*> >::type,not_found> ));
    BOOST_MPL_ASSERT_NOT(( is_same<BOOST_DEDUCED_TYPENAME find<M,mpl::pair<long,long*> >::type,not_found> ));
    BOOST_MPL_ASSERT_NOT(( is_same<BOOST_DEDUCED_TYPENAME find<M,mpl::pair<char,char*> >::type,not_found> ));
    BOOST_MPL_ASSERT(( is_same<BOOST_DEDUCED_TYPENAME find<M,int>::type,not_found> ));
};

MPL_TEST_CASE()
{
    typedef map< mpl::pair<int,int*> > map_of_1_pair;
    typedef begin<map_of_1_pair>::type iter_to_1_pair;
    
    BOOST_MPL_ASSERT((
        is_same<
             deref<iter_to_1_pair>::type
           , mpl::pair<int,int*>
        >
    ));
    
    typedef map<
        mpl::pair<int,int*>
      , mpl::pair<long,long*>
      , mpl::pair<char,char*>
    > mymap;
    
    test<mymap>();
    test<mymap::type>();
}

MPL_TEST_CASE()
{
    typedef mpl::erase_key<
        mpl::map<
            mpl::pair<char, double>
          , mpl::pair<int, float>
        >
      , char
    >::type int_to_float_map;

    typedef mpl::insert<
        int_to_float_map
      , mpl::pair<char, long>
    >::type with_char_too;

    BOOST_MPL_ASSERT((
        boost::is_same<
            mpl::at<with_char_too, char>::type
          , long
        >
    ));
}
