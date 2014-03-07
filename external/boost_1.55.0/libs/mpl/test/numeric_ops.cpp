
// Copyright Aleksey Gurtovoy 2003-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: numeric_ops.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/arithmetic.hpp>
#include <boost/mpl/comparison.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/long.hpp>
#include <boost/mpl/aux_/test.hpp>

struct complex_tag : int_<10> {};

template< typename Re, typename Im > struct complex
{
    typedef complex_tag tag;
    typedef complex type;
    typedef Re real;
    typedef Im imag;
};

template< typename C > struct real : C::real {};
template< typename C > struct imag : C::imag {};

namespace boost { namespace mpl {

template<> struct BOOST_MPL_AUX_NUMERIC_CAST< integral_c_tag,complex_tag >
{
    template< typename N > struct apply
        : complex< N, integral_c< typename N::value_type, 0 > >
    {
    };
};

template<>
struct plus_impl< complex_tag,complex_tag >
{
    template< typename N1, typename N2 > struct apply
        : complex<
              plus< typename N1::real, typename N2::real >
            , plus< typename N1::imag, typename N2::imag >
            >
    {
    };
};

template<>
struct times_impl< complex_tag,complex_tag >
{
    template< typename N1, typename N2 > struct apply
        : complex<
              minus< 
                  times< typename N1::real, typename N2::real >
                , times< typename N1::imag, typename N2::imag >
                >
            , plus<
                  times< typename N1::real, typename N2::imag >
                , times< typename N1::imag, typename N2::real >
                >
            >
    {
    };
};

template<>
struct equal_to_impl< complex_tag,complex_tag >
{
    template< typename N1, typename N2 > struct apply
        : and_<
              equal_to< typename N1::real, typename N2::real >
            , equal_to< typename N1::imag, typename N2::imag >
            >
    {
    };
};

}}


typedef int_<2> i;
typedef complex< int_<5>, int_<-1> > c1;
typedef complex< int_<-5>, int_<1> > c2;

MPL_TEST_CASE()
{
    typedef plus<c1,c2>::type r1;
    MPL_ASSERT_RELATION( real<r1>::value, ==, 0 );
    MPL_ASSERT_RELATION( imag<r1>::value, ==, 0 );

    typedef plus<c1,c1>::type r2;
    MPL_ASSERT_RELATION( real<r2>::value, ==, 10 );
    MPL_ASSERT_RELATION( imag<r2>::value, ==, -2 );

    typedef plus<c2,c2>::type r3;
    MPL_ASSERT_RELATION( real<r3>::value, ==, -10 );
    MPL_ASSERT_RELATION( imag<r3>::value, ==, 2 );

#if !BOOST_WORKAROUND(BOOST_MSVC, < 1300)
    typedef plus<c1,i>::type r4;
    MPL_ASSERT_RELATION( real<r4>::value, ==, 7 );
    MPL_ASSERT_RELATION( imag<r4>::value, ==, -1 );

    typedef plus<i,c2>::type r5;
    MPL_ASSERT_RELATION( real<r5>::value, ==, -3 );
    MPL_ASSERT_RELATION( imag<r5>::value, ==, 1 );
#endif
}

MPL_TEST_CASE()
{
    typedef times<c1,c2>::type r1;
    MPL_ASSERT_RELATION( real<r1>::value, ==, -24 );
    MPL_ASSERT_RELATION( imag<r1>::value, ==, 10 );

    typedef times<c1,c1>::type r2;
    MPL_ASSERT_RELATION( real<r2>::value, ==, 24 );
    MPL_ASSERT_RELATION( imag<r2>::value, ==, -10 );

    typedef times<c2,c2>::type r3;
    MPL_ASSERT_RELATION( real<r3>::value, ==, 24 );
    MPL_ASSERT_RELATION( imag<r3>::value, ==, -10 );

#if !BOOST_WORKAROUND(BOOST_MSVC, < 1300)
    typedef times<c1,i>::type r4;
    MPL_ASSERT_RELATION( real<r4>::value, ==, 10 );
    MPL_ASSERT_RELATION( imag<r4>::value, ==, -2 );

    typedef times<i,c2>::type r5;
    MPL_ASSERT_RELATION( real<r5>::value, ==, -10 );
    MPL_ASSERT_RELATION( imag<r5>::value, ==, 2 );
#endif
}

MPL_TEST_CASE()
{
    MPL_ASSERT(( equal_to<c1,c1> ));
    MPL_ASSERT(( equal_to<c2,c2> ));
    MPL_ASSERT_NOT(( equal_to<c1,c2> ));

    MPL_ASSERT(( equal_to<c1, complex< long_<5>, long_<-1> > > ));

#if !BOOST_WORKAROUND(BOOST_MSVC, < 1300)
    MPL_ASSERT_NOT(( equal_to<c1,i> ));
    MPL_ASSERT_NOT(( equal_to<i,c2> ));
#endif
}
