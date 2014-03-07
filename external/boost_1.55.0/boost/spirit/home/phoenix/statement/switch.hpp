/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_STATEMENT_SWITCH_HPP
#define PHOENIX_STATEMENT_SWITCH_HPP

#include <boost/spirit/home/phoenix/core/composite.hpp>
#include <boost/spirit/home/phoenix/core/compose.hpp>
#include <boost/spirit/home/phoenix/core/nothing.hpp>
#include <boost/spirit/home/phoenix/statement/detail/switch_eval.hpp>
#include <boost/spirit/home/phoenix/statement/detail/switch.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/not.hpp>

namespace boost { namespace phoenix
{
    template <typename Derived, typename Actor>
    struct switch_case_base 
    {
        typedef Derived derived_t;
        typedef Actor actor_t;
        typedef typename Actor::no_nullary no_nullary;

        template <typename Env>
        struct result
        {
            typedef typename Actor::eval_type::template result<Env>::type type;
        };

        switch_case_base(Actor const& actor)
            : actor(actor) {}

        template <typename Env>
        typename result<Env>::type
        eval(Env const& env) const
        {
            return actor.eval(env);
        }

        Actor actor;
    };

    template <typename Actor, typename K, K Value>
    struct switch_case : switch_case_base<switch_case<Actor, K, Value>, Actor>
    {
        typedef switch_case_base<switch_case<Actor, K, Value>, Actor> base_t;
        static K const value = Value;
        static bool const is_default = false;

        switch_case(Actor const& actor)
            : base_t(actor) {}
    };

    template <typename Actor>
    struct default_case : switch_case_base<default_case<Actor>, Actor>
    {
        typedef switch_case_base<default_case<Actor>, Actor> base_t;
        static bool const is_default = true;

        default_case(Actor const& actor)
            : base_t(actor) {}
    };

    template <typename Cond>
    struct switch_gen
    {
        switch_gen(Cond const& cond)
            : cond(cond) {}

        template <typename Cases>
        typename lazy_enable_if<
            fusion::traits::is_sequence<Cases>
          , detail::switch_composite_actor<Cond, Cases>
        >::type
        operator[](Cases const& cases) const
        {
            typedef typename
                detail::switch_composite<Cond, Cases>
            switch_composite;
            return switch_composite::eval(cond, cases);
        }

        template <typename D, typename A>
        actor<typename detail::
            switch_composite<Cond, fusion::vector<actor<D> > >::type>
        operator[](switch_case_base<D, A> const& case_) const
        {
            typedef typename
                detail::switch_composite<Cond, fusion::vector<actor<D> > >
            switch_composite;
            return switch_composite::eval(cond,
                fusion::vector<actor<D> >(static_cast<D const&>(case_)));
        }

        Cond cond;
    };

    template <typename Cond>
    inline switch_gen<typename as_actor<Cond>::type>
    switch_(Cond const& cond)
    {
        return switch_gen<typename as_actor<Cond>::type>(
            as_actor<Cond>::convert(cond));
    }

    template <int N, typename A0>
    switch_case<typename as_actor<A0>::type, int, N>
    case_(A0 const& _0)
    {
        return switch_case<typename as_actor<A0>::type, int, N>
            (as_actor<A0>::convert(_0));
    }

    template <typename A0>
    default_case<typename as_actor<A0>::type>
    default_(A0 const& _0)
    {
        return default_case<typename as_actor<A0>::type>
            (as_actor<A0>::convert(_0));
    }

    template <typename D0, typename A0, typename D1, typename A1>
    inline typename detail::compose_case_a<D0, D1>::type
    operator,(
        switch_case_base<D0, A0> const& _0
      , switch_case_base<D1, A1> const& _1
    )
    {
        return detail::compose_case_a<D0, D1>::eval(
            static_cast<D0 const&>(_0)
          , static_cast<D1 const&>(_1)
        );
    }

    template <typename Seq, typename D, typename A>
    inline typename
        lazy_enable_if<
            fusion::traits::is_sequence<Seq>
          , detail::compose_case_b<Seq, D>
        >::type
    operator,(Seq const& seq, switch_case_base<D, A> const& case_)
    {
        return detail::compose_case_b<Seq, D>::eval(
            seq, static_cast<D const&>(case_));
    }

    // Implementation of routines in detail/switch.hpp that depend on
    // the completeness of default_case.
    namespace detail {
        template <typename Cases>
        typename ensure_default<Cases>::type
        ensure_default<Cases>::eval(Cases const& cases, mpl::false_)
        {
            actor<default_case<actor<null_actor> > > default_
              = default_case<actor<null_actor> >(nothing);
            return fusion::push_front(cases, default_);
        }
    }
}}

#endif
