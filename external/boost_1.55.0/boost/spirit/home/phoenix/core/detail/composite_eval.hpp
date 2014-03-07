/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PP_IS_ITERATING
#ifndef PHOENIX_CORE_DETAIL_COMPOSITE_EVAL_HPP
#define PHOENIX_CORE_DETAIL_COMPOSITE_EVAL_HPP

#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>

    namespace detail
    {
        template <int N>
        struct composite_eval;

        template <>
        struct composite_eval<0>
        {
            template <typename Composite, typename Env>
            struct result
            {
                typedef typename Composite::eval_policy_type::
                    template result<Env>::type
                type;
            };

            template <typename RT, typename Composite, typename Env>
            static RT
            call(Composite const& /*composite*/, Env const& env)
            {
                typedef typename Composite::eval_policy_type eval_policy_type;
                return eval_policy_type::template eval<RT>(env);
            }
        };

        template <typename Actor, typename Env>
        struct eval_is_actor
            : is_actor<typename Actor::template result<Env>::type> {};

        template <typename Actor, typename Env>
        struct eval_is_void
            : is_same<typename Actor::template result<Env>::type, fusion::void_> {};
    }

#define PHOENIX_GET_ACTOR_TYPE(z, n, data)                                      \
    typedef                                                                     \
        typename fusion::result_of::value_at_c<Composite, n>::type              \
    BOOST_PP_CAT(actor, n);

#define PHOENIX_GET_ACTOR(z, n, data)                                           \
    fusion::at_c<n>(composite)

#define BOOST_PP_ITERATION_PARAMS_1                                             \
    (3, (1, PHOENIX_COMPOSITE_LIMIT,                                            \
    "boost/spirit/home/phoenix/core/detail/composite_eval.hpp"))
#include BOOST_PP_ITERATE()

#undef PHOENIX_GET_ACTOR
#undef PHOENIX_GET_ACTOR_TYPE
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  Preprocessor vertical repetition code
//
///////////////////////////////////////////////////////////////////////////////
#else // defined(BOOST_PP_IS_ITERATING)

#define N BOOST_PP_ITERATION()

    namespace detail
    {
        template <>
        struct composite_eval<N>
        {
            template <typename Composite, typename Env>
            struct result
            {
                BOOST_PP_REPEAT(N, PHOENIX_GET_ACTOR_TYPE, _)

                typedef typename
                    Composite::eval_policy_type::template result<
                        Env, BOOST_PP_ENUM_PARAMS(N, actor)
                    >::type
                type;
            };

            template <typename RT, typename Composite, typename Env>
            static RT
            call(Composite const& composite, Env const& env)
            {
                typedef typename Composite::eval_policy_type eval_policy_type;
                return eval_policy_type::template eval<RT>(
                    env, BOOST_PP_ENUM(N, PHOENIX_GET_ACTOR, _));
            }
        };
    }

#undef N
#endif // defined(BOOST_PP_IS_ITERATING)


