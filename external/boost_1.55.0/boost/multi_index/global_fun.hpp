/* Copyright 2003-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#ifndef BOOST_MULTI_INDEX_GLOBAL_FUN_HPP
#define BOOST_MULTI_INDEX_GLOBAL_FUN_HPP

#if defined(_MSC_VER)&&(_MSC_VER>=1200)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/utility/enable_if.hpp>

#if !defined(BOOST_NO_SFINAE)
#include <boost/type_traits/is_convertible.hpp>
#endif

namespace boost{

template<class T> class reference_wrapper; /* fwd decl. */

namespace multi_index{

namespace detail{

/* global_fun is a read-only key extractor from Value based on a given global
 * (or static member) function with signature:
 *
 *   Type f([const] Value [&]);
 *
 * Additionally, global_fun  and const_global_fun are overloaded to support
 * referece_wrappers of Value and "chained pointers" to Value's. By chained
 * pointer to T we  mean a type P such that, given a p of Type P
 *   *...n...*x is convertible to T&, for some n>=1.
 * Examples of chained pointers are raw and smart pointers, iterators and
 * arbitrary combinations of these (vg. T** or auto_ptr<T*>.)
 */

/* NB. Some overloads of operator() have an extra dummy parameter int=0.
 * This disambiguator serves several purposes:
 *  - Without it, MSVC++ 6.0 incorrectly regards some overloads as
 *    specializations of a previous member function template.
 *  - MSVC++ 6.0/7.0 seem to incorrectly treat some different memfuns
 *    as if they have the same signature.
 *  - If remove_const is broken due to lack of PTS, int=0 avoids the
 *    declaration of memfuns with identical signature.
 */

template<class Value,typename Type,Type (*PtrToFunction)(Value)>
struct const_ref_global_fun_base
{
  typedef typename remove_reference<Type>::type result_type;

  template<typename ChainedPtr>

#if !defined(BOOST_NO_SFINAE)
  typename disable_if<
    is_convertible<const ChainedPtr&,Value>,Type>::type
#else
  Type
#endif

  operator()(const ChainedPtr& x)const
  {
    return operator()(*x);
  }

  Type operator()(Value x)const
  {
    return PtrToFunction(x);
  }

  Type operator()(
    const reference_wrapper<
      typename remove_reference<Value>::type>& x)const
  { 
    return operator()(x.get());
  }

  Type operator()(
    const reference_wrapper<
      typename remove_const<
        typename remove_reference<Value>::type>::type>& x,int=0)const
  { 
    return operator()(x.get());
  }
};

template<class Value,typename Type,Type (*PtrToFunction)(Value)>
struct non_const_ref_global_fun_base
{
  typedef typename remove_reference<Type>::type result_type;

  template<typename ChainedPtr>

#if !defined(BOOST_NO_SFINAE)
  typename disable_if<
    is_convertible<ChainedPtr&,Value>,Type>::type
#else
  Type
#endif

  operator()(const ChainedPtr& x)const
  {
    return operator()(*x);
  }

  Type operator()(Value x)const
  {
    return PtrToFunction(x);
  }

  Type operator()(
    const reference_wrapper<
      typename remove_reference<Value>::type>& x)const
  { 
    return operator()(x.get());
  }
};

template<class Value,typename Type,Type (*PtrToFunction)(Value)>
struct non_ref_global_fun_base
{
  typedef typename remove_reference<Type>::type result_type;

  template<typename ChainedPtr>

#if !defined(BOOST_NO_SFINAE)
  typename disable_if<
    is_convertible<const ChainedPtr&,const Value&>,Type>::type
#else
  Type
#endif

  operator()(const ChainedPtr& x)const
  {
    return operator()(*x);
  }

  Type operator()(const Value& x)const
  {
    return PtrToFunction(x);
  }

  Type operator()(const reference_wrapper<const Value>& x)const
  { 
    return operator()(x.get());
  }

  Type operator()(
    const reference_wrapper<
      typename remove_const<Value>::type>& x,int=0)const
  { 
    return operator()(x.get());
  }
};

} /* namespace multi_index::detail */

template<class Value,typename Type,Type (*PtrToFunction)(Value)>
struct global_fun:
  mpl::if_c<
    is_reference<Value>::value,
    typename mpl::if_c<
      is_const<typename remove_reference<Value>::type>::value,
      detail::const_ref_global_fun_base<Value,Type,PtrToFunction>,
      detail::non_const_ref_global_fun_base<Value,Type,PtrToFunction>
    >::type,
    detail::non_ref_global_fun_base<Value,Type,PtrToFunction>
  >::type
{
};

} /* namespace multi_index */

} /* namespace boost */

#endif
