/*=============================================================================
    Copyright (c) 2005-2007 Dan Marsden
    Copyright (c) 2005-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef PHOENIX_STATEMENT_DETAIL_CATCH_COMPOSITE_HPP
#define PHOENIX_STATEMENT_DETAIL_CATCH_COMPOSITE_HPP

#include <boost/spirit/home/phoenix/core/actor.hpp>
#include <boost/spirit/home/phoenix/core/composite.hpp>
#include <boost/spirit/home/phoenix/core/compose.hpp>

namespace boost { namespace phoenix {
namespace detail
{
    struct catch_composite_eval
    {
        template<typename Env, typename Actor>
        struct result :
            eval_result<typename Actor::eval_type, Env> {};

        template<typename Rt, typename Env, typename Actor>
        static typename result<Env,Actor>::type
        eval(const Env& env, Actor& actor)
        {
            return actor.eval(env);
        }
    };

    template<typename Exception, typename Actor>
    struct catch_composite :
        composite<catch_composite_eval, fusion::vector<Actor> >
    {
        catch_composite(const Actor& actor)
            : composite<catch_composite_eval, fusion::vector<Actor> >(actor) { }

        typedef Exception exception_type;
    };

    template<typename Exception, typename Actor>
    struct as_catch_actor
    {
        typedef catch_composite<
            Exception,
            Actor> comp;

        typedef actor<comp> type;
    };

    template<typename Exception, typename Actor>
    inline typename as_catch_actor<Exception, Actor>::type
    catch_actor(const Actor& actor)
    {
        return catch_composite<Exception,Actor>(actor);
    }
}
}}

#endif
