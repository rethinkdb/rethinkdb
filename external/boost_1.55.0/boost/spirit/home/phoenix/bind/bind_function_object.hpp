/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_BIND_BIND_FUNCTION_OBJECT_HPP
#define PHOENIX_BIND_BIND_FUNCTION_OBJECT_HPP

#include <boost/spirit/home/phoenix/core/compose.hpp>
#include <boost/spirit/home/phoenix/core/detail/function_eval.hpp>

namespace boost { namespace phoenix
{
    template <typename F>
    inline actor<typename as_composite<detail::function_eval<0>, F>::type>
    bind(F const& f)
    {
        return compose<detail::function_eval<0> >(f);
    }

    template <typename F, typename A0>
    inline actor<typename as_composite<detail::function_eval<1>, F, A0>::type>
    bind(F const& f, A0 const& _0)
    {
        return compose<detail::function_eval<1> >(f, _0);
    }

    template <typename F, typename A0, typename A1>
    inline actor<typename as_composite<detail::function_eval<2>, F, A0, A1>::type>
    bind(F const& f, A0 const& _0, A1 const& _1)
    {
        return compose<detail::function_eval<2> >(f, _0, _1);
    }

    //  Bring in the rest of the function object binders
    #include <boost/spirit/home/phoenix/bind/detail/bind_function_object.hpp>
}}

#endif
