/*=============================================================================
    Copyright (c) 2005-2007 Dan Marsden
    Copyright (c) 2005-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef PHOENIX_STATEMENT_THROW_HPP
#define PHOENIX_STATEMENT_THROW_HPP

#include <boost/spirit/home/phoenix/core/actor.hpp>
#include <boost/spirit/home/phoenix/core/composite.hpp>
#include <boost/spirit/home/phoenix/core/compose.hpp>

#include <boost/mpl/bool.hpp>

namespace boost { namespace phoenix {

    struct throw_new_eval
    {
        template<typename Env, typename ThrowExpr>
        struct result
        {
            typedef void type;
        };

        template<typename Rt, typename Env, typename ThrowExpr>
        static void 
        eval(const Env& env, ThrowExpr& throwExpr)
        {
            throw throwExpr.eval(env);
        }
    };

    struct throw_again_eval
    {
        typedef mpl::false_ no_nullary;

        template<typename Env>
        struct result
        {
            typedef void type;
        };

        template<typename Env>
        void eval(const Env&) const
        {
            throw;
        }
    };

    inline actor<throw_again_eval> throw_()
    {
        return throw_again_eval();
    }

    template<typename Actor>
    actor<typename as_composite<throw_new_eval, Actor>::type>
    throw_(const Actor& a)
    {
        return compose<throw_new_eval>(a);
    }
}}

#endif
