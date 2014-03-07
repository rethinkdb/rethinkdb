
// Copyright Aleksey Gurtovoy 2000-2004
// Copyright David Abrahams 2003-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: copy.cpp 55647 2009-08-18 05:00:17Z agurtovoy $
// $Date: 2009-08-17 22:00:17 -0700 (Mon, 17 Aug 2009) $
// $Revision: 55647 $

#include <boost/mpl/copy.hpp>

#include <boost/mpl/vector/vector20_c.hpp>
#include <boost/mpl/vector/vector0.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/front_inserter.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/mpl/less.hpp>

#include <boost/mpl/begin_end_fwd.hpp>
#include <boost/mpl/size_fwd.hpp>
#include <boost/mpl/empty_fwd.hpp>
#include <boost/mpl/front_fwd.hpp>
#include <boost/mpl/insert_fwd.hpp>
#include <boost/mpl/insert_range_fwd.hpp>
#include <boost/mpl/erase_fwd.hpp>
#include <boost/mpl/clear_fwd.hpp>
#include <boost/mpl/push_back_fwd.hpp>
#include <boost/mpl/pop_back_fwd.hpp>
#include <boost/mpl/back_fwd.hpp>

#include <boost/mpl/aux_/test.hpp>

MPL_TEST_CASE()
{
    typedef vector10_c<int,9,8,7,6,5,4,3,2,1,0> answer;
    typedef copy<
          range_c<int,0,10>
        , mpl::front_inserter< vector0<> >
        >::type result;

    MPL_ASSERT_RELATION( size<result>::value, ==, 10 );
    MPL_ASSERT(( equal< result,answer > ));
}

MPL_TEST_CASE()
{
    typedef vector10_c<int,10,11,12,13,14,15,16,17,18,19> numbers;
    typedef reverse_copy<
          range_c<int,0,10>
        , mpl::front_inserter<numbers>
        >::type result;

    MPL_ASSERT_RELATION( size<result>::value, ==,  20 );
    MPL_ASSERT(( equal< result,range_c<int,0,20> > ));
}

struct push_back_only_tag {};

template< typename Seq >
struct push_back_only
{
    typedef push_back_only_tag tag;
    typedef Seq seq;
};

namespace boost { namespace mpl {

template<>
struct begin_impl< ::push_back_only_tag >
{
    template< typename Seq > struct apply
        : begin< typename Seq::seq >
    {
    };
};

template<>
struct end_impl< ::push_back_only_tag >
{
    template< typename Seq > struct apply
        : end< typename Seq::seq >
    {
    };
};

template<>
struct size_impl< ::push_back_only_tag >
{
    template< typename Seq > struct apply
        : size< typename Seq::seq >
    {
    };
};

template<>
struct empty_impl< ::push_back_only_tag >
{
    template< typename Seq > struct apply
        : empty< typename Seq::seq >
    {
    };
};

template<>
struct front_impl< ::push_back_only_tag >
{
    template< typename Seq > struct apply
        : front< typename Seq::seq >
    {
    };
};

template<>
struct insert_impl< ::push_back_only_tag >
{
    template< typename Seq, typename Pos, typename X > struct apply
    {
        typedef ::push_back_only<
            typename insert< typename Seq::seq, Pos, X >::type
        > type;
    };
};

template<>
struct insert_range_impl< ::push_back_only_tag >
{
    template< typename Seq, typename Pos, typename X > struct apply
    {
        typedef ::push_back_only<
            typename insert_range< typename Seq::seq, Pos, X >::type
        > type;
    };
};

template<>
struct erase_impl< ::push_back_only_tag >
{
    template< typename Seq, typename Iter1, typename Iter2 > struct apply
    {
        typedef ::push_back_only<
            typename erase< typename Seq::seq, Iter1, Iter2 >::type
        > type;
    };
};

template<>
struct clear_impl< ::push_back_only_tag >
{
    template< typename Seq > struct apply
    {
        typedef ::push_back_only<
            typename clear< typename Seq::seq >::type
        > type;
    };
};

template<>
struct push_back_impl< ::push_back_only_tag >
{
    template< typename Seq, typename X > struct apply
    {
        typedef ::push_back_only<
            typename push_back< typename Seq::seq, X >::type
        > type;
    };
};

template<>
struct pop_back_impl< ::push_back_only_tag >
{
    template< typename Seq > struct apply
    {
        typedef ::push_back_only<
            typename pop_back< typename Seq::seq >::type
        > type;
    };
};

template<>
struct back_impl< ::push_back_only_tag >
{
    template< typename Seq > struct apply
        : back< typename Seq::seq >
    {
    };
};

template<>
struct has_push_back_impl< ::push_back_only_tag >
{
    template< typename Seq > struct apply
        : less< size<Seq>,int_<10> >
    {
    };
};

}}

MPL_TEST_CASE()
{
    typedef vector10_c<int,0,1,2,3,4,5,6,7,8,9> numbers;
    typedef copy< push_back_only< numbers > >::type result;

    MPL_ASSERT((equal< numbers, result >));
}
