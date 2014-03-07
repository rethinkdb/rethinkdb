//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TR1_VERIFY_FUNCTOR_HPP
#define BOOST_TR1_VERIFY_FUNCTOR_HPP

#include <boost/type_traits/is_base_and_derived.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/static_assert.hpp>
#include "verify_return.hpp"

template <class F, class R>
void verify_nullary_functor(F f, R)
{
   verify_return_type(f(), R());
}

template <class F, class B>
void verify_unary_functor(F f, B)
{
#ifndef NO_INHERT_TEST
   BOOST_STATIC_ASSERT( (::boost::is_base_and_derived<B, F>::value));
#endif
   typedef typename B::argument_type arg_type;
   typedef typename B::result_type result_type;
   typedef typename F::argument_type b_arg_type;
   typedef typename F::result_type b_result_type;
   BOOST_STATIC_ASSERT((::boost::is_same<arg_type, b_arg_type>::value));
   BOOST_STATIC_ASSERT((::boost::is_same<result_type, b_result_type>::value));
   static arg_type a;

   typedef typename boost::remove_reference<result_type>::type result_value;
   verify_return_type(f(a), result_value());
}

template <class F, class B>
void verify_field_functor(F f, B)
{
   // same as verify_unary_functor, but no nested result_type
   // because it depends on the argument type.
   typedef typename B::argument_type arg_type;
   typedef typename B::result_type result_type;
   static arg_type a;

   typedef typename boost::remove_reference<result_type>::type result_value;
   verify_return_type(f(a), result_value());
   result_type r = f(a);
   (void)r;
}


template <class F, class B>
void verify_binary_functor(F f, B)
{
#ifndef NO_INHERT_TEST
   BOOST_STATIC_ASSERT( (::boost::is_base_and_derived<B, F>::value));
#endif
   typedef typename B::first_argument_type b_arg1_type;
   typedef typename B::second_argument_type b_arg2_type;
   typedef typename B::result_type b_result_type;
   typedef typename F::first_argument_type arg1_type;
   typedef typename F::second_argument_type arg2_type;
   typedef typename F::result_type result_type;
   BOOST_STATIC_ASSERT((::boost::is_same<arg1_type, b_arg1_type>::value));
   BOOST_STATIC_ASSERT((::boost::is_same<arg2_type, b_arg2_type>::value));
   BOOST_STATIC_ASSERT((::boost::is_same<result_type, b_result_type>::value));
   static arg1_type a;
   static arg2_type b;
   verify_return_type(f(a, b), result_type());
}

#endif

