
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: apply.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/apply.hpp>
#include <boost/mpl/lambda.hpp>
#include <boost/mpl/plus.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/aux_/test.hpp>


template< typename T > struct std_vector
{
#if defined(BOOST_MPL_CFG_NO_IMPLICIT_METAFUNCTIONS)
    typedef std_vector type;
    BOOST_MPL_AUX_LAMBDA_SUPPORT(1, std_vector, (T))
#endif
};


MPL_TEST_CASE()
{
    typedef plus<int_<2>,int_<3> > plus1;
    typedef lambda<plus1>::type plus2;
    MPL_ASSERT(( is_same< plus1,plus2 > ));

    typedef lambda<std_vector<int> >::type v;
    MPL_ASSERT(( is_same< v,std_vector<int> > ));

    typedef lambda<std_vector<_1> >::type make_vector;
    typedef apply_wrap1<make_vector,int>::type v_int;
    MPL_ASSERT(( is_same< v_int,std_vector<int> > ));
}

MPL_TEST_CASE()
{
    typedef plus<_1,_2> plus_fun;
    typedef apply2<plus_fun,int_<2>,int_<3> >::type res;

    MPL_ASSERT_RELATION( res::value, ==, 5 );
}

MPL_TEST_CASE()
{
    typedef apply1<_1, plus<_1,_2> >::type plus_fun;
    MPL_ASSERT(( is_same< plus_fun,plus<_1,_2> > ));

    typedef apply2<plus_fun,int_<2>,int_<3> >::type res;
    MPL_ASSERT_RELATION( res::value, ==, 5 );
}

MPL_TEST_CASE()
{
    typedef lambda< lambda<_1> >::type make_lambda;
    typedef apply_wrap1< make_lambda,std_vector<int> >::type v;
    MPL_ASSERT(( is_same< v,std_vector<int> > ));

    typedef apply_wrap1< make_lambda,std_vector<_1> >::type make_vector;
    typedef apply_wrap1< make_vector,int >::type v_int;
    MPL_ASSERT(( is_same< v_int,std_vector<int> > ));
}

MPL_TEST_CASE()
{
    typedef apply1< _1, std_vector<int> >::type v;
    MPL_ASSERT(( is_same< v,std_vector<int> > ));

    typedef apply1< _1, std_vector<_1> >::type v_lambda;
    typedef apply1<v_lambda,int>::type v_int;
    MPL_ASSERT(( is_same< v,std_vector<int> > ));
}

MPL_TEST_CASE()
{
    typedef apply1< lambda<_1>, std_vector<int> >::type v;
    MPL_ASSERT(( is_same< v,std_vector<int> > ));

    typedef apply1< lambda<_1>, std_vector<_1> >::type make_vector;
    typedef apply_wrap1< make_vector,int >::type v_int;
    MPL_ASSERT(( is_same< v,std_vector<int> > ));
}

MPL_TEST_CASE()
{
    typedef apply1< lambda<_1>, plus<_1,_2> >::type plus_fun;
    typedef apply_wrap2< plus_fun,int_<2>,int_<3> >::type res;

    MPL_ASSERT_RELATION( res::value, ==, 5 );
}

MPL_TEST_CASE()
{
    typedef bind2<plus<>,_1,_1> b1;
    typedef lambda<b1>::type b2;
    MPL_ASSERT(( is_same< b1,b2 > ));
}

MPL_TEST_CASE()
{
#if !BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3003))
    typedef lambda< lambda< bind2<plus<>,_1,_1> > >::type make_lambda;
    typedef apply_wrap1< make_lambda::type, int_<5> >::type res;
    MPL_ASSERT_RELATION( res::value, ==, 10 );
#endif
}

MPL_TEST_CASE()
{
    typedef apply1< bind2<plus<>,_1,_1>, int_<5> >::type res;
    MPL_ASSERT_RELATION( res::value, ==, 10 );
}

MPL_TEST_CASE()
{
    typedef apply1<_1, lambda<plus<_1,_2> > >::type plus_fun;
    typedef apply_wrap2< plus_fun::type, int_<2>,int_<3> >::type res;

    MPL_ASSERT_RELATION( res::value, ==, 5 );
}
