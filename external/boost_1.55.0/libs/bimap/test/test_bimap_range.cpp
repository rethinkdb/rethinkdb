// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  VC++ 8.0 warns on usage of certain Standard Library and API functions that
//  can be cause buffer overruns or other possible security issues if misused.
//  See http://msdn.microsoft.com/msdnmag/issues/05/05/SafeCandC/default.aspx
//  But the wording of the warning is misleading and unsettling, there are no
//  portable alternative functions, and VC++ 8.0's own libraries use the
//  functions in question. So turn off the warnings.
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

#include <boost/config.hpp>

// Boost.Test
#include <boost/test/minimal.hpp>

#include <boost/config.hpp>

#include <algorithm>

#include <boost/range/functions.hpp>
#include <boost/range/metafunctions.hpp>

#include <boost/bimap/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/bimap/support/lambda.hpp>


template< class ForwardReadableRange, class UnaryFunctor >
UnaryFunctor for_each(const ForwardReadableRange & r, UnaryFunctor func)
{
    typedef typename 
    boost::range_const_iterator<ForwardReadableRange>::type const_iterator;

    for(const_iterator i= boost::begin(r), iend= boost::end(r); i!=iend; ++i )
    {
        func(*i);
    }

    return func;
}

struct do_something_with_a_pair
{
    template< class Pair >
    void operator()(const Pair & p)
    {
        BOOST_CHECK( p.first && p.second );
    }
};

int test_bimap_range()
{
    using namespace boost::bimaps;

    typedef bimap< double, multiset_of<int> > bm_type;


    bm_type bm;
    bm.insert( bm_type::value_type(1.1 , 1) );
    bm.insert( bm_type::value_type(2.2 , 2) );
    bm.insert( bm_type::value_type(3.3 , 3) );
    bm.insert( bm_type::value_type(4.4 , 4) );


    for_each( bm.left.range( 1.0 < _key, _key < 5.0 ),
              do_something_with_a_pair() );

    for_each( bm.right.range( unbounded, _key <= 2 ),
              do_something_with_a_pair() );


    // left range
    {

    bm_type::left_range_type r = bm.left.range( 2.0 < _key, _key < 4.0 );
    BOOST_CHECK( ! boost::empty(r) );
    BOOST_CHECK( boost::begin(r) == bm.left.upper_bound(2.0) );
    BOOST_CHECK( boost::end(r)   == bm.left.lower_bound(4.0) );

    }

    // right range
    {

    bm_type::right_range_type r = bm.right.range( 2 <= _key, _key <= 3 );
    BOOST_CHECK( ! boost::empty(r) );
    BOOST_CHECK( boost::begin(r) == bm.right.lower_bound(2) );
    BOOST_CHECK( boost::end(r)   == bm.right.upper_bound(3) );

    }

    // const range from range
    {

    bm_type:: left_const_range_type lr = bm. left.range( unbounded, _key < 4.0 );
    bm_type::right_const_range_type rr = bm.right.range( 2 < _key ,  unbounded );

    }

    const bm_type & cbm = bm;

    // left const range
    {
    bm_type:: left_const_range_type r = cbm.left.range( unbounded, unbounded );
    BOOST_CHECK( ! boost::empty(r) );
    BOOST_CHECK( boost::begin(r) == cbm.left.begin() );

    }

    // right const range
    {

    bm_type::right_const_range_type r = cbm.right.range( 1 < _key, _key < 1 );
    BOOST_CHECK( boost::empty(r) );

    }

    return 0;
}
//]

int test_main( int, char* [] )
{
    test_bimap_range();
    return 0;
}

