/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_STATEMENT_DO_WHILE_HPP
#define PHOENIX_STATEMENT_DO_WHILE_HPP

#include <boost/spirit/home/phoenix/core/composite.hpp>
#include <boost/spirit/home/phoenix/core/compose.hpp>

namespace boost { namespace phoenix
{
    struct do_while_eval
    {
        template <typename Env, typename Cond, typename Do>
        struct result
        {
            typedef void type;
        };

        template <typename RT, typename Env, typename Cond, typename Do>
        static void
        eval(Env const& env, Cond& cond, Do& do_)
        {
            do
                do_.eval(env);
            while (cond.eval(env));
        }
    };

    template <typename Do>
    struct do_while_gen
    {
        do_while_gen(Do const& do_)
            : do_(do_) {}

        template <typename Cond>
        actor<typename as_composite<do_while_eval, Cond, Do>::type>
        while_(Cond const& cond) const
        {
            return compose<do_while_eval>(cond, do_);
        }

        Do do_;
    };

    struct do_gen
    {
        template <typename Do>
        do_while_gen<Do>
        operator[](Do const& do_) const
        {
            return do_while_gen<Do>(do_);
        }
    };

    do_gen const do_ = do_gen();
}}

#endif
