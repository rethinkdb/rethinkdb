
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: fold.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/fold.hpp>
#include <boost/mpl/reverse_fold.hpp>
//#include <boost/mpl/vector.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/list_c.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/next.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/less.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/size.hpp>
#include <boost/type_traits/is_float.hpp>

#include <boost/mpl/aux_/test.hpp>

MPL_TEST_CASE()
{
    typedef list<long,float,short,double,float,long,long double> types;
    typedef fold<
          types
        , int_<0>
        , if_< boost::is_float<_2>,next<_1>,_1 >
        >::type number_of_floats;

    MPL_ASSERT_RELATION( number_of_floats::value, ==, 4 );
}

MPL_TEST_CASE()
{
    typedef list_c<int,5,-1,0,-7,-2,0,-5,4> numbers;
    typedef list_c<int,-1,-7,-2,-5> negatives;
    typedef reverse_fold<
          numbers
        , list_c<int>
        , if_< less< _2,int_<0> >, push_front<_1,_2>, _1 >
        >::type result;

    MPL_ASSERT(( equal< result,negatives,equal_to<_1,_2> > ));
}
