
// Copyright Aleksey Gurtovoy 2001-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: lambda.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/logical.hpp>
#include <boost/mpl/comparison.hpp>
#include <boost/mpl/lambda.hpp>
#include <boost/mpl/size_t.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/sizeof.hpp>
#include <boost/mpl/apply.hpp>

#include <boost/mpl/aux_/test.hpp>

#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_float.hpp>

struct my
{
    char a[100];
};

MPL_TEST_CASE()
{
    // !(x == char) && !(x == double) || sizeof(x) > 8
    typedef lambda<
        or_<
              and_<
                    not_< boost::is_same<_1, char> >
                  , not_< boost::is_float<_1> >
                  >
            , greater< sizeof_<_1>, mpl::size_t<8> >
            >
        >::type f;

    MPL_ASSERT_NOT(( apply_wrap1<f,char> ));
    MPL_ASSERT_NOT(( apply_wrap1<f,double> ));
    MPL_ASSERT(( apply_wrap1<f,long> ));
    MPL_ASSERT(( apply_wrap1<f,my> ));
}

MPL_TEST_CASE()
{
    // x == y || x == my || sizeof(x) == sizeof(y)
    typedef lambda<
        or_< 
              boost::is_same<_1, _2>
            , boost::is_same<_2, my>
            , equal_to< sizeof_<_1>, sizeof_<_2> >
            >
        >::type f;

    MPL_ASSERT_NOT(( apply_wrap2<f,double,char> ));
    MPL_ASSERT_NOT(( apply_wrap2<f,my,int> ));
    MPL_ASSERT_NOT(( apply_wrap2<f,my,char[99]> ));
    MPL_ASSERT(( apply_wrap2<f,int,int> ));
    MPL_ASSERT(( apply_wrap2<f,my,my> ));
    MPL_ASSERT(( apply_wrap2<f,signed long, unsigned long> ));
}

MPL_TEST_CASE()
{
    // bind <-> lambda interaction
    typedef lambda< less<_1,_2> >::type pred;
    typedef bind2< pred, _1, int_<4> > f;
    
    MPL_ASSERT(( apply_wrap1< f,int_<3> > ));
}
