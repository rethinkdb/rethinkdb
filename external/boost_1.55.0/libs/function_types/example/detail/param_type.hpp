
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

// Metafunction to compute optimal parameter type for argument forwarding. 

// This header is not an FT example in itself -- it's used by some of them to 
// optimize argument forwarding. 
//
// For more details see 'fast_mem_fn.hpp' in this directory or the documentation
// of the CallTraits utility [1].
//
//
// References
// ==========
//
// [1] http://www.boost.org/libs/utility/call_traits.htm

#ifndef BOOST_UTILITY_PARAM_TYPE_HPP_INCLUDED
#define BOOST_UTILITY_PARAM_TYPE_HPP_INCLUDED

#include <boost/config.hpp>

#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/add_reference.hpp>

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>

#include <boost/mpl/aux_/lambda_support.hpp>
// #include <boost/type_traits/detail/template_arity_spec.hpp>

// namespace boost
namespace example
{
  namespace mpl = boost::mpl;

  // namespace utility
  // {
    namespace param_type_detail
    {
      template<typename T>
      struct by_ref_cond
      {
        typedef by_ref_cond type;
        BOOST_STATIC_CONSTANT(bool,value = sizeof(void*) < sizeof(T));
      };

      template<typename T>
      struct add_ref_to_const
        : boost::add_reference< typename boost::add_const<T>::type >
      { };
    }

    template<typename T>
    struct param_type
      : mpl::eval_if< param_type_detail::by_ref_cond<T> 
                    , param_type_detail::add_ref_to_const<T>, mpl::identity<T> >
    { 
      BOOST_MPL_AUX_LAMBDA_SUPPORT(1,param_type,(T))
    };
  // }
  // BOOST_TT_AUX_TEMPLATE_ARITY_SPEC(1,utility::param_type)
}

#endif

