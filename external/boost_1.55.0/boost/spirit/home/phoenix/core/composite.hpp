/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_CORE_COMPOSITE_HPP
#define PHOENIX_CORE_COMPOSITE_HPP

#include <boost/spirit/home/phoenix/core/actor.hpp>
#include <boost/spirit/home/phoenix/core/is_actor.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/size.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/or.hpp>

namespace boost { namespace phoenix
{
    namespace detail
    {
        template <int N>
        struct composite_eval;
        
        struct compute_no_nullary
        {
            template <typename State, typename T>
            struct apply
            {
                typedef typename 
                    mpl::or_<typename T::no_nullary, State>::type
                type;
            };
        };
    }

    template <typename EvalPolicy, typename EvalTuple>
    struct composite : EvalTuple
    {
        typedef EvalTuple base_type;
        typedef composite<EvalPolicy, EvalTuple> self_type;
        typedef EvalPolicy eval_policy_type;
        
        typedef typename
            mpl::fold<
                EvalTuple
              , mpl::false_
              , detail::compute_no_nullary
            >::type
        no_nullary;

        template <typename Env>
        struct result
        {
            typedef
                typename detail::composite_eval<
                    fusion::result_of::size<base_type>::value>::
                template result<self_type, Env>::type
            type;
        };

        composite()
            : base_type() {}

        composite(base_type const& base)
            : base_type(base) {}

        template <typename U0>
        composite(U0& _0)
            : base_type(_0) {}

        template <typename U0, typename U1>
        composite(U0& _0, U1& _1)
            : base_type(_0, _1) {}

        template <typename Env>
        typename result<Env>::type
        eval(Env const& env) const
        {
            typedef typename result<Env>::type return_type;
            return detail::
                composite_eval<fusion::result_of::size<base_type>::value>::template
                    call<return_type>(*this, env);
        }

        //  Bring in the rest of the constructors
        #include <boost/spirit/home/phoenix/core/detail/composite.hpp>
    };

    //  Bring in the detail::composite_eval<0..N> definitions
    #include <boost/spirit/home/phoenix/core/detail/composite_eval.hpp>
}}

#endif
