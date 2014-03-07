/*=============================================================================
    Copyright (c) 2005-2007 Dan Marsden
    Copyright (c) 2005-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PP_IS_ITERATING
#ifndef PHOENIX_STATEMENT_DETAIL_CATCH_EVAL_HPP
#define PHOENIX_STATEMENT_DETAIL_CATCH_EVAL_HPP

#include <boost/spirit/home/phoenix/core/limits.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>
#include <boost/preprocessor/repetition/enum_params_with_defaults.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/dec.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/mpl/void.hpp>

namespace boost { namespace phoenix {
    class catch_eval
    {
    public:
        template<typename Env, typename TryBody, 
                 BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(PHOENIX_CATCH_LIMIT, typename CatchBody, mpl::void_)>
        struct result
        {
            typedef void type;
        };

#define BOOST_PP_ITERATION_PARAMS_1                                                        \
        (3, (1, PHOENIX_CATCH_LIMIT, "boost/spirit/home/phoenix/statement/detail/catch_eval.hpp"))

#include BOOST_PP_ITERATE()

    };
}}

#endif

#elif BOOST_PP_ITERATION_DEPTH() == 1

#define PHOENIX_ITERATION BOOST_PP_ITERATION()

        template<typename Rt, typename Env, typename TryBody,
                 BOOST_PP_ENUM_PARAMS(PHOENIX_ITERATION, typename CatchBody)>
        static void eval(
            const Env& env, TryBody& tryBody,
            BOOST_PP_ENUM_BINARY_PARAMS(PHOENIX_ITERATION, CatchBody, catchBody))
        {
            try
            {
                tryBody.eval(env);
            }

#define BOOST_PP_ITERATION_PARAMS_2                                                                         \
        (3, (0, BOOST_PP_DEC(PHOENIX_ITERATION), "boost/spirit/home/phoenix/statement/detail/catch_eval.hpp"))

#include BOOST_PP_ITERATE()

        }

#undef PHOENIX_ITERATION

#elif BOOST_PP_ITERATION_DEPTH() == 2

#define PHOENIX_ITERATION BOOST_PP_ITERATION()

        catch(typename BOOST_PP_CAT(CatchBody, PHOENIX_ITERATION)::exception_type&)
        {                                                         
            BOOST_PP_CAT(catchBody, PHOENIX_ITERATION).eval(env);
        }

#undef PHOENIX_ITERATION

#endif
