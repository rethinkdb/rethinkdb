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

// Boost.Bimap Example
//-----------------------------------------------------------------------------

#include <boost/config.hpp>

#include <iostream>

#include <boost/range/functions.hpp>
#include <boost/range/metafunctions.hpp>

//[ code_bimap_and_boost_range_functions

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

template< class ForwardReadableRange, class Predicate >
typename boost::range_difference<ForwardReadableRange>::type
    count_if(const ForwardReadableRange & r, Predicate pred)
{
    typedef typename
    boost::range_const_iterator<ForwardReadableRange>::type const_iterator;

    typename boost::range_difference<ForwardReadableRange>::type c = 0;

    for( const_iterator i = boost::begin(r), iend = boost::end(r); i != iend; ++i )
    {
        if( pred(*i) ) ++c;
    }

    return c;
}
//]

#include <boost/bimap/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/bimap/support/lambda.hpp>
#include <boost/bind.hpp>

using namespace boost::bimaps;
using namespace boost;

//[ code_bimap_and_boost_range

struct pair_printer
{
    pair_printer(std::ostream & o) : os(o) {}
    template< class Pair >
    void operator()(const Pair & p)
    {
        os << "(" << p.first << "," << p.second << ")";
    }
    private:
    std::ostream & os;
};

struct second_extractor
{
    template< class Pair >
    const typename Pair::second_type & operator()(const Pair & p)
    {
        return p.second;
    }
};

int main()
{
    typedef bimap< double, multiset_of<int> > bm_type;

    bm_type bm;
    bm.insert( bm_type::value_type(2.5 , 1) );
    bm.insert( bm_type::value_type(3.1 , 2) );
    //...
    bm.insert( bm_type::value_type(6.4 , 4) );
    bm.insert( bm_type::value_type(1.7 , 2) );

    // Print all the elements of the left map view

    for_each( bm.left, pair_printer(std::cout) );

    // Print a range of elements of the right map view

    for_each( bm.right.range( 2 <= _key, _key < 6 ), pair_printer(std::cout) );

    // Count the number of elements where the data is equal to 2 from a
    // range of elements of the left map view

    count_if( bm.left.range( 2.3 < _key, _key < 5.4 ),
              bind<int>( second_extractor(), _1 ) == 2 );

    return 0;
}
//]

