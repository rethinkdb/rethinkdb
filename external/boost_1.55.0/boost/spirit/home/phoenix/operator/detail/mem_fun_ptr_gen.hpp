/*=============================================================================
    Copyright (c) 2005-2007 Dan Marsden
    Copyright (c) 2005-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PP_IS_ITERATING
#ifndef PHOENIX_OPERATOR_DETAIL_MEM_FUN_PTR_GEN_HPP
#define PHOENIX_OPERATOR_DETAIL_MEM_FUN_PTR_GEN_HPP

#include <boost/spirit/home/phoenix/core/composite.hpp>
#include <boost/spirit/home/phoenix/core/compose.hpp>
#include <boost/spirit/home/phoenix/core/as_actor.hpp>
#include <boost/spirit/home/phoenix/core/limits.hpp>
#include <boost/spirit/home/phoenix/core/actor.hpp>

#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/arithmetic/dec.hpp>

#include <boost/spirit/home/phoenix/operator/detail/mem_fun_ptr_eval.hpp>

namespace boost { namespace phoenix {
    template<typename Actor, typename MemFunPtr>
    struct mem_fun_ptr_gen
    {
        mem_fun_ptr_gen(
            const Actor& actor, MemFunPtr memFunPtr)
            : mActor(actor), mMemFunPtr(memFunPtr) { }

        actor<typename as_composite<mem_fun_ptr_eval, Actor, typename as_actor<MemFunPtr>::type>::type>
        operator()() const
        {
            return compose<mem_fun_ptr_eval>(
                mActor, as_actor<MemFunPtr>::convert(mMemFunPtr));
        }

#define BOOST_PP_ITERATION_PARAMS_1                                                        \
        (3, (1, BOOST_PP_DEC(BOOST_PP_DEC(PHOENIX_MEMBER_LIMIT)), "boost/spirit/home/phoenix/operator/detail/mem_fun_ptr_gen.hpp"))

#include BOOST_PP_ITERATE()

        Actor mActor;
        MemFunPtr mMemFunPtr;
    };
}}

#endif
#else

#define PHOENIX_ITERATION BOOST_PP_ITERATION()

        template<BOOST_PP_ENUM_PARAMS(PHOENIX_ITERATION, typename Arg)>
        actor<typename as_composite<
            mem_fun_ptr_eval, Actor, typename as_actor<MemFunPtr>::type,
            BOOST_PP_ENUM_PARAMS(PHOENIX_ITERATION, Arg)>::type>
        operator()(
            BOOST_PP_ENUM_BINARY_PARAMS(PHOENIX_ITERATION, const Arg, &arg)) const
        {
            return compose<mem_fun_ptr_eval>(
                mActor, as_actor<MemFunPtr>::convert(mMemFunPtr),
                BOOST_PP_ENUM_PARAMS(PHOENIX_ITERATION, arg));
        }

#undef PHOENIX_ITERATION

#endif
