
// Copyright Aleksey Gurtovoy 2000-2004
// Copyright David Abrahams 2003-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: transform.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/transform.hpp>

#include <boost/mpl/list.hpp>
#include <boost/mpl/list_c.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/plus.hpp>
#include <boost/mpl/aux_/test.hpp>
#include <boost/mpl/aux_/config/gcc.hpp>
#include <boost/mpl/aux_/config/workaround.hpp>

#include <boost/type_traits/add_pointer.hpp>


MPL_TEST_CASE()
{
    typedef list<char,short,int,long,float,double> types;
    typedef list<char*,short*,int*,long*,float*,double*> pointers;
    
    typedef transform1< types,add_pointer<_1> >::type result;
    MPL_ASSERT(( equal<result,pointers> ));
}

MPL_TEST_CASE()
{
    typedef list_c<long,0,2,4,6,8,10> evens;
    typedef list_c<long,2,3,5,7,11,13> primes;
    typedef list_c<long,2,5,9,13,19,23> sums;

    typedef transform2< evens, primes, plus<> >::type result;
    MPL_ASSERT(( equal< result,sums,equal_to<_1,_2> > ));

#if !defined(BOOST_MPL_CFG_NO_HAS_XXX)
    typedef transform< evens, primes, plus<> >::type result2;
    MPL_ASSERT(( is_same<result2,result> ));
#endif
}
