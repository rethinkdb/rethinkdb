
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_OLD_DETAIL_ARG_H
#define BOOST_COROUTINES_OLD_DETAIL_ARG_H

#include <boost/config.hpp>
#include <boost/preprocessor/arithmetic/add.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/type_traits/function_traits.hpp>

#include <boost/coroutine/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines {
namespace detail {

template<
    typename Signature,
    int arity = function_traits< Signature >::arity >
struct arg;

template< typename Signature >
struct arg< Signature, 1 >
{
    typedef typename function_traits< Signature >::arg1_type    type;
};

#define BOOST_CONTEXT_TUPLE_COMMA(n) BOOST_PP_COMMA_IF(BOOST_PP_SUB(n,1))
#define BOOST_CONTEXT_TUPLE_TYPE(z,n,unused) \
    BOOST_CONTEXT_TUPLE_COMMA(n) typename function_traits< Signature >::BOOST_PP_CAT(BOOST_PP_CAT(arg,n),_type)
#define BOOST_CONTEXT_TUPLE_TYPES(n) BOOST_PP_REPEAT_FROM_TO(1,BOOST_PP_ADD(n,1),BOOST_CONTEXT_TUPLE_TYPE,~)
#define BOOST_CONTEXT_TUPLE(z,n,unused) \
template< typename Signature > \
struct arg< Signature, n > \
{ \
    typedef tuple< BOOST_CONTEXT_TUPLE_TYPES(n) >   type; \
};
BOOST_PP_REPEAT_FROM_TO(2,11,BOOST_CONTEXT_TUPLE,~)
#undef BOOST_CONTEXT_TUPLE
#undef BOOST_CONTEXT_TUPLE_TYPES
#undef BOOST_CONTEXT_TUPLE_TYPE
#undef BOOST_CONTEXT_TUPLE_COMMA

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES_OLD_DETAIL_ARG_H
