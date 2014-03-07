// Boost.TypeErasure library
//
// Copyright 2012 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: null.hpp 79190 2012-06-30 20:07:23Z steven_watanabe $

#if !defined(BOOST_PP_IS_ITERATING)

#ifndef BOOST_TYPE_ERASURE_DETAIL_NULL_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_DETAIL_NULL_HPP_INCLUDED

#include <boost/throw_exception.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/type_erasure/config.hpp>
#include <boost/type_erasure/exception.hpp>

namespace boost {
namespace type_erasure {
namespace detail {

template<class Sig>
struct null_throw;

template<class Concept>
struct get_null_vtable_entry {
    typedef ::boost::type_erasure::detail::null_throw<
        typename ::boost::remove_pointer<typename Concept::type>::type> type;
};

#define BOOST_PP_FILENAME_1 <boost/type_erasure/detail/null.hpp>
#define BOOST_PP_ITERATION_LIMITS (0, BOOST_TYPE_ERASURE_MAX_ARITY)
#include BOOST_PP_ITERATE()

}
}
}

#endif

#else

#define N BOOST_PP_ITERATION()

template<class R BOOST_PP_ENUM_TRAILING_PARAMS(N, class T)>
struct null_throw<R(BOOST_PP_ENUM_PARAMS(N, T))> {
    static R value(BOOST_PP_ENUM_PARAMS(N, T)) { 
        BOOST_THROW_EXCEPTION(::boost::type_erasure::bad_function_call());
    }
};

#undef N

#endif
