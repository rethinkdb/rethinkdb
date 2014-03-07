/*=============================================================================
    Copyright (c) 2005-2012 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_PP_IS_ITERATING)
#if !defined(BOOST_FUSION_SEQUENCE_DEQUE_DETAIL_DEQUE_KEYED_VALUES_CALL_04122006_2211)
#define BOOST_FUSION_SEQUENCE_DEQUE_DETAIL_DEQUE_KEYED_VALUES_CALL_04122006_2211

#if defined(BOOST_FUSION_HAS_VARIADIC_DEQUE)
#error "C++03 only! This file should not have been included"
#endif

#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum_shifted_params.hpp>
#include <boost/preprocessor/repetition/enum_shifted.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>

#define FUSION_DEQUE_KEYED_VALUES_FORWARD(z, n, _)    \
   std::forward<BOOST_PP_CAT(T_, n)>(BOOST_PP_CAT(t, n))

#define BOOST_PP_FILENAME_1 \
    <boost/fusion/container/deque/detail/cpp03/deque_keyed_values_call.hpp>
#define BOOST_PP_ITERATION_LIMITS (1, FUSION_MAX_DEQUE_SIZE)
#include BOOST_PP_ITERATE()

#undef FUSION_DEQUE_KEYED_VALUES_FORWARD
#endif
#else

#define N BOOST_PP_ITERATION()

        static type construct(BOOST_PP_ENUM_BINARY_PARAMS(N, typename add_reference<typename add_const<T, >::type>::type t))
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
        #if N > 1
                        , BOOST_PP_ENUM_SHIFTED_PARAMS(N, T)
        #endif
                        >::construct(BOOST_PP_ENUM_SHIFTED_PARAMS(N, t)));
        }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <BOOST_PP_ENUM_PARAMS(N, typename T_)>
        static type forward_(BOOST_PP_ENUM_BINARY_PARAMS(N, T_, && t))
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
        #if N > 1
                        , BOOST_PP_ENUM_SHIFTED_PARAMS(N, T_)
        #endif
                        >::forward_(BOOST_PP_ENUM_SHIFTED(N, FUSION_DEQUE_KEYED_VALUES_FORWARD, _)));
        }
#endif

#undef N
#endif
