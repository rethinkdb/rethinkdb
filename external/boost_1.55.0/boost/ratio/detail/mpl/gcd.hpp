////////////////////////////////////////////////////////////////////
//
// Copyright Vicente J. Botet Escriba 2010
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.
//
////////////////////////////////////////////////////////////////////
#ifndef BOOST_MPL_GCD_HPP_INCLUDED
#define BOOST_MPL_GCD_HPP_INCLUDED

#include <boost/mpl/integral_c.hpp>
#include <boost/ratio/detail/mpl/abs.hpp>
#include <boost/mpl/aux_/largest_int.hpp>
#include <boost/mpl/aux_/na_spec.hpp>
#include <boost/mpl/aux_/lambda_support.hpp>
#include <boost/mpl/aux_/config/eti.hpp>
#include <boost/mpl/aux_/config/integral.hpp>
#include <boost/mpl/aux_/config/static_constant.hpp>
#include <boost/mpl/aux_/config/dependent_nttp.hpp>
#include <boost/cstdint.hpp>

#if    !defined(BOOST_MPL_CFG_NO_NESTED_VALUE_ARITHMETIC_2) \
    && !defined(BOOST_MPL_PREPROCESSING_MODE) \
    && ( defined(BOOST_MSVC) \
        || BOOST_WORKAROUND(__EDG_VERSION__, <= 238) \
        )

#   define BOOST_MPL_CFG_NO_NESTED_VALUE_ARITHMETIC_2

#endif

namespace boost { namespace mpl {

template< typename Tag1, typename Tag2 > struct gcd_impl;

template< typename T > struct gcd_tag
{
    typedef typename T::tag type;
};

template<
      typename BOOST_MPL_AUX_NA_PARAM(N1)
    , typename BOOST_MPL_AUX_NA_PARAM(N2)
    >
struct gcd
    : gcd_impl<
          typename gcd_tag<N1>::type
        , typename gcd_tag<N2>::type
        >::template apply<N1, N2>::type
{
    BOOST_MPL_AUX_LAMBDA_SUPPORT(2, gcd, (N1, N2))
};

BOOST_MPL_AUX_NA_SPEC(2, gcd)

template<
      typename T
    , T n1
    , T n2
    >
struct gcd_c
    : gcd<integral_c<T,n1>,integral_c<T,n2> >
{
};

namespace aux {

  // Workaround for error: the type of partial specialization template parameter constant "n2"
    // depends on another template parameter
    // Note: this solution could be wrong for n1 or n2 = [2**63 .. 2**64-1]
#if defined(BOOST_MPL_CFG_NO_DEPENDENT_NONTYPE_PARAMETER_IN_PARTIAL_SPEC)

  template< typename T1, boost::intmax_t n1, bool n1_is_0
                  , typename T2, boost::intmax_t n2, bool n2_is_0 >
      struct gcd_aux
          : gcd_aux<T2, n2, n2==0, T1, (n1 % n2), (n1 % n2)==0>
      {};

      template <typename T1, boost::intmax_t n1, typename T2, boost::intmax_t n2>
      struct gcd_aux<T1, n1, false, T2, n2, true> : integral_c<T1, n1>
      {};

      template <typename T1, boost::intmax_t n1, typename T2, boost::intmax_t n2, bool C>
      struct gcd_aux<T1, n1, true, T2, n2, C> : integral_c<T2, n2>
      {};

#else // defined(BOOST_MPL_CFG_NO_DEPENDENT_NONTYPE_PARAMETER_IN_PARTIAL_SPEC)

    template< typename T1, T1 n1, bool n1_is_0, typename T2, T2 n2, bool n2_is_0 >
    struct gcd_aux

        : gcd_aux<T2, n2, n2==0,
                    typename aux::largest_int<T1, T2>::type,
                    //~ T1,
                    (n1 % n2), (n1 % n2)==0>
    {};

    template <typename T1, T1 n1, typename T2, T2 n2>
    struct gcd_aux<T1, n1, false, T2, n2, true> : integral_c<T1, n1>
    {};

    template <typename T1, T1 n1, typename T2, T2 n2, bool C>
    struct gcd_aux<T1, n1, true, T2, n2, C> : integral_c<T2, n2>
    {};
#endif // defined(BOOST_MPL_CFG_NO_DEPENDENT_NONTYPE_PARAMETER_IN_PARTIAL_SPEC)
}

template<>
struct gcd_impl<integral_c_tag, integral_c_tag>
{
    template< typename N1, typename N2 > struct apply
        : abs<aux::gcd_aux< typename N1::value_type, N1::value, N1::value==0,
                        typename N2::value_type, N2::value, N2::value==0  > >
    {
    };
};

}}

#endif // BOOST_MPL_GCD_HPP_INCLUDED
