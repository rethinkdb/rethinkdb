
// Copyright Aleksey Gurtovoy 2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: partition.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/partition.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/back_inserter.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/mpl/modulus.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/aux_/test.hpp>

template< typename N > struct is_odd
    : modulus< N, int_<2> > 
{
    BOOST_MPL_AUX_LAMBDA_SUPPORT(1, is_odd, (N))
};


MPL_TEST_CASE()
{
    typedef partition<
          range_c<int,0,10> 
        , is_odd<_1>
        , mpl::back_inserter< vector<> >
        , mpl::back_inserter< vector<> >
        >::type r;

    MPL_ASSERT(( equal< r::first, vector_c<int,1,3,5,7,9> > ));
    MPL_ASSERT(( equal< r::second, vector_c<int,0,2,4,6,8> > ));
}
