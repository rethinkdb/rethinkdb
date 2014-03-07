/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_CORE_IS_ACTOR_HPP
#define PHOENIX_CORE_IS_ACTOR_HPP

#include <boost/spirit/home/phoenix/core/actor.hpp>
#include <boost/mpl/bool.hpp>

namespace boost { namespace phoenix
{
///////////////////////////////////////////////////////////////////////////////
//
//  is_actor<T>
//
//      Tests if T is an actor. Evaluates to mpl::true_ or mpl::false_
//
///////////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct is_actor : mpl::false_ {};

    template <typename Base>
    struct is_actor<actor<Base> > : mpl::true_ {};
}}

#endif
