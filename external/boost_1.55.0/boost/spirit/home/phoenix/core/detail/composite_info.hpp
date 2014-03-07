/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_CORE_DETAIL_COMPOSITE_INFO_HPP
#define PHOENIX_CORE_DETAIL_COMPOSITE_INFO_HPP

#include <boost/mpl/if.hpp>
#include <boost/mpl/greater.hpp>
#include <boost/mpl/int.hpp>
#include <boost/spirit/home/fusion/algorithm/fold.hpp>

namespace boost { namespace phoenix { namespace detail
{
    struct max_nesting
    {
        template <typename T, typename State>
        struct result :
            mpl::if_<
                mpl::greater<typename T::nesting, State>
              , typename T::nesting
              , State
            >
        {};
    };

    template <typename ActorTuple>
    struct compute_nesting
    {
        typedef typename
            fusion::meta::fold<ActorTuple, mpl::int_<0>, max_nesting>::type
        type;
    };

}}}

#endif

