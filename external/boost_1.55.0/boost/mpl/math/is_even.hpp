
#ifndef BOOST_MPL_MATH_IS_EVEN_HPP_INCLUDED
#define BOOST_MPL_MATH_IS_EVEN_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: is_even.hpp 49267 2008-10-11 06:19:02Z agurtovoy $
// $Date: 2008-10-10 23:19:02 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49267 $

#include <boost/mpl/bool.hpp>
#include <boost/mpl/aux_/na_spec.hpp>
#include <boost/mpl/aux_/lambda_support.hpp>
#include <boost/mpl/aux_/config/msvc.hpp>
#include <boost/mpl/aux_/config/workaround.hpp>

namespace boost { namespace mpl {

#if BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
namespace aux
{
  template <class N>
  struct is_even_base
  {
      enum { value = (N::value % 2) == 0 };
      typedef bool_<value> type;
  };
}
#endif 

template<
      typename BOOST_MPL_AUX_NA_PARAM(N)
    >
struct is_even
#if BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
  : aux::is_even_base<N>::type
#else
  : bool_<((N::value % 2) == 0)>
#endif 
{
    BOOST_MPL_AUX_LAMBDA_SUPPORT(1,is_even,(N))
};

BOOST_MPL_AUX_NA_SPEC(1, is_even)

}}

#endif // BOOST_MPL_MATH_IS_EVEN_HPP_INCLUDED
