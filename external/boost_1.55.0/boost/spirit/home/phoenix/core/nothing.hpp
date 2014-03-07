/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_CORE_NOTHING_HPP
#define PHOENIX_CORE_NOTHING_HPP

#include <boost/spirit/home/phoenix/core/actor.hpp>
#include <boost/mpl/bool.hpp>

namespace boost { namespace phoenix
{
///////////////////////////////////////////////////////////////////////////////
//
//  null_actor
//
//      A actor that does nothing (a "bum", if you will :-).
//
///////////////////////////////////////////////////////////////////////////////
    struct null_actor
    {
        typedef mpl::false_ no_nullary;

        template <typename Env>
        struct result
        {
            typedef void type;
        };

        template <typename Env>
        void
        eval(Env const&) const
        {
        }
    };

    actor<null_actor> const nothing = null_actor();
}}

#endif
