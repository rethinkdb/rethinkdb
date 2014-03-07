/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_OBJECT_CONSTRUCT_HPP
#define PHOENIX_OBJECT_CONSTRUCT_HPP

#include <boost/spirit/home/phoenix/core/compose.hpp>
#include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>

namespace boost { namespace phoenix
{
    namespace detail
    {
        template <typename T>
        struct construct_eval
        {
            template <typename Env,
                BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(
                    PHOENIX_COMPOSITE_LIMIT, typename T, fusion::void_)>
            struct result
            {
                typedef T type;
            };

            template <typename RT, typename Env>
            static RT
            eval(Env const& /*env*/)
            {
                return RT();
            }

            template <typename RT, typename Env, typename A0>
            static RT
            eval(Env const& env, A0& _0)
            {
                return RT(_0.eval(env));
            }

            template <typename RT
                , typename Env, typename A0, typename A1>
            static RT
            eval(Env const& env, A0& _0, A1& _1)
            {
                return RT(_0.eval(env), _1.eval(env));
            }

            //  Bring in the rest of the evals
            #include <boost/spirit/home/phoenix/object/detail/construct_eval.hpp>
        };
    }

    template <typename T>
    inline actor<typename as_composite<detail::construct_eval<T> >::type>
    construct()
    {
        return compose<detail::construct_eval<T> >();
    }

    template <typename T, typename A0>
    inline actor<typename as_composite<detail::construct_eval<T>, A0>::type>
    construct(A0 const& _0)
    {
        return compose<detail::construct_eval<T> >(_0);
    }

    template <typename T, typename A0, typename A1>
    inline actor<typename as_composite<detail::construct_eval<T>, A0, A1>::type>
    construct(A0 const& _0, A1 const& _1)
    {
        return compose<detail::construct_eval<T> >(_0, _1);
    }

    //  Bring in the rest of the new_ functions
    #include <boost/spirit/home/phoenix/object/detail/construct.hpp>
}}

#endif
