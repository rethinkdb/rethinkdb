/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_BIND_BIND_FUNCTION_HPP
#define PHOENIX_BIND_BIND_FUNCTION_HPP

#include <boost/spirit/home/phoenix/core/compose.hpp>
#include <boost/spirit/home/phoenix/core/detail/function_eval.hpp>
#include <boost/spirit/home/phoenix/bind/detail/function_ptr.hpp>

namespace boost { namespace phoenix
{
    template <typename RT>
    inline actor<
        typename as_composite<
            detail::function_eval<0>
          , detail::function_ptr<0, RT, RT(*)()>
        >::type>
    bind(RT(*f)())
    {
        typedef detail::function_ptr<0, RT, RT(*)()> fp_type;
        return compose<detail::function_eval<0> >(fp_type(f));
    }

    template <typename RT, typename T0, typename A0>
    inline actor<
        typename as_composite<
            detail::function_eval<1>
          , detail::function_ptr<1, RT, RT(*)(T0)>
          , A0
        >::type>
    bind(RT(*f)(T0), A0 const& _0)
    {
        typedef detail::function_ptr<1, RT, RT(*)(T0)> fp_type;
        return compose<detail::function_eval<1> >(fp_type(f), _0);
    }

    template <typename RT, typename T0, typename T1, typename A0, typename A1>
    inline actor<
        typename as_composite<
            detail::function_eval<2>
          , detail::function_ptr<2, RT, RT(*)(T0, T1)>
          , A0, A1
        >::type>
    bind(RT(*f)(T0, T1), A0 const& _0, A1 const& _1)
    {
        typedef detail::function_ptr<2, RT, RT(*)(T0, T1)> fp_type;
        return compose<detail::function_eval<2> >(fp_type(f), _0, _1);
    }

    //  Bring in the rest of the function binders
    #include <boost/spirit/home/phoenix/bind/detail/bind_function.hpp>
}}

#endif
