
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: quote.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/quote.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/aux_/test.hpp>

template< typename T > struct f1
{
    typedef T type;
};

template<
      typename T1, typename T2, typename T3, typename T4, typename T5
    >
struct f5
{
#if !defined(BOOST_MPL_CFG_NO_IMPLICIT_METAFUNCTIONS)
    // no 'type' member!
#else
    typedef f5 type;
#endif
};

MPL_TEST_CASE()
{
    typedef quote1<f1>::apply<int>::type t1;
    typedef quote5<f5>::apply<char,short,int,long,float>::type t5;
    
    MPL_ASSERT(( boost::is_same< t1, int > ));
    MPL_ASSERT(( boost::is_same< t5, f5<char,short,int,long,float> > ));
}
