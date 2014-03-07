/* Copyright 2006-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#ifndef BOOST_FLYWEIGHT_DETAIL_DEFAULT_VALUE_POLICY_HPP
#define BOOST_FLYWEIGHT_DETAIL_DEFAULT_VALUE_POLICY_HPP

#if defined(_MSC_VER)&&(_MSC_VER>=1200)
#pragma once
#endif

#include <boost/flyweight/detail/value_tag.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>

/* Default value policy: the key is the same as the value.
 */

namespace boost{

namespace flyweights{

namespace detail{

template<typename Value>
struct default_value_policy:value_marker
{
  typedef Value key_type;
  typedef Value value_type;

  struct rep_type
  {
  /* template ctors */

#define BOOST_FLYWEIGHT_PERFECT_FWD_NAME explicit rep_type
#define BOOST_FLYWEIGHT_PERFECT_FWD_BODY(n) \
  :x(BOOST_PP_ENUM_PARAMS(n,t)){}
#include <boost/flyweight/detail/perfect_fwd.hpp>

    operator const value_type&()const{return x;}

    value_type x;
  };

  static void construct_value(const rep_type&){}
  static void copy_value(const rep_type&){}
};

} /* namespace flyweights::detail */

} /* namespace flyweights */

} /* namespace boost */

#endif
