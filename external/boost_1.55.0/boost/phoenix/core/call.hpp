/*==============================================================================
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2011 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PHOENIX_CORE_CALL_HPP
#define BOOST_PHOENIX_CORE_CALL_HPP

#include <boost/phoenix/core/limits.hpp>
#include <boost/phoenix/core/environment.hpp>
#include <boost/proto/proto_fwd.hpp>
#include <boost/proto/traits.hpp>
#include <boost/proto/transform/impl.hpp>

namespace boost { namespace phoenix
{
    namespace detail
    {
        template <
            typename Fun
          , typename Expr
          , typename State
          , typename Data
          , long Arity = proto::arity_of<Expr>::value
        >
        struct call_impl;
        
        template <typename Fun, typename Expr, typename State, typename Data>
        struct call_impl<Fun, Expr, State, Data, 0>
            : proto::transform_impl<Expr, State, Data>
        {
            typedef
                typename boost::phoenix::result_of::context<State, Data>::type
                context_type;
            
            typedef
                typename boost::result_of<
                    Fun(Expr, context_type)
                >::type
                result_type;
            
            result_type operator()(
                typename call_impl::expr_param e
              , typename call_impl::state_param s
              , typename call_impl::data_param d
            ) const
            {
                return Fun()(e, boost::phoenix::context(s, d));
            }
        };
    }

    template <typename Fun, typename Dummy = void>
    struct call
        : proto::transform<call<Fun> >
    {
        template <typename Expr, typename State, typename Data>
        struct impl
            : detail::call_impl<Fun, Expr, State, Data>
        {};
    };

    #include <boost/phoenix/core/detail/call.hpp>

}
    namespace proto
    {
        template <typename Fun, typename Dummy>
        struct is_callable<phoenix::call<Fun, Dummy> > : mpl::true_ {};
    }
}

#endif
