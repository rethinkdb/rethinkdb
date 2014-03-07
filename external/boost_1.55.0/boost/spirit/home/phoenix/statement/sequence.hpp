/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_STATEMENT_SEQUENCE_HPP
#define PHOENIX_STATEMENT_SEQUENCE_HPP

#include <boost/spirit/home/phoenix/core/composite.hpp>
#include <boost/spirit/home/phoenix/core/compose.hpp>

namespace boost { namespace phoenix
{
    struct sequence_eval
    {
        template <typename Env, typename A0, typename A1>
        struct result
        {
            typedef void type;
        };

        template <typename RT, typename Env, typename A0, typename A1>
        static void
        eval(Env const& env, A0& a0, A1& a1)
        {
            a0.eval(env);
            a1.eval(env);
        }
    };
    
    namespace detail
    {
        template <typename BaseT0, typename BaseT1>
        struct comma_result
        {
            typedef actor<
                typename as_composite<
                    sequence_eval
                  , actor<BaseT0>
                  , actor<BaseT1>
                >::type
            > type;
        };
    }

    template <typename BaseT0, typename BaseT1>
    inline typename detail::comma_result<BaseT0, BaseT1>::type
    operator,(actor<BaseT0> const& a0, actor<BaseT1> const& a1)
    {
        return compose<sequence_eval>(a0, a1);
    }
}}

#endif
