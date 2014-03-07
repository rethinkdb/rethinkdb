/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_CORE_ACTOR_HPP
#define PHOENIX_CORE_ACTOR_HPP

#include <boost/spirit/home/phoenix/core/limits.hpp>

#if !defined(BOOST_RESULT_OF_NUM_ARGS)
# define BOOST_RESULT_OF_NUM_ARGS PHOENIX_ACTOR_LIMIT
#elif (BOOST_RESULT_OF_NUM_ARGS < PHOENIX_ACTOR_LIMIT)
# error "BOOST_RESULT_OF_NUM_ARGS < PHOENIX_ACTOR_LIMIT"
#endif

#include <boost/spirit/home/phoenix/core/basic_environment.hpp>
#include <boost/mpl/min.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/utility/result_of.hpp>

namespace boost { namespace phoenix
{
    // phoenix::void_ is the same as fusion::void_
    typedef fusion::void_ void_;

    namespace detail
    {
        //  Forward declarations. These will come in when we get to the
        //  operator module, yet, the actor's assignment operator and index
        //  operator are required to be members.

        template <typename T0, typename T1>
        struct make_assign_composite;

        template <typename T0, typename T1>
        struct make_index_composite;

        template <typename BaseT0, typename BaseT1>
        struct comma_result;

        // error no arguments supplied
        struct error_expecting_arguments
        {
            template <typename T>
            error_expecting_arguments(T const&) {}
        };
    }

    template <typename Eval, typename Env>
    struct eval_result
    {
        typedef typename Eval::template result<Env>::type type;
    };

#if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable: 4522) // multiple assignment operators specified warning
#endif

    template <typename Eval>
    struct actor : Eval
    {
        typedef actor<Eval> self_type;
        typedef Eval eval_type;

        template <class Sig> struct result {};

        actor()
            : Eval() {}

        actor(Eval const& base)
            : Eval(base) {}

        template <typename T0>
        explicit actor(T0 const& _0)
            : Eval(_0) {}

        template <typename T0, typename T1>
        actor(T0 const& _0, T1 const& _1)
            : Eval(_0, _1) {}

        typedef typename
            mpl::eval_if<
                typename Eval::no_nullary // avoid calling eval_result when this is true
              , mpl::identity<detail::error_expecting_arguments>
              , eval_result<eval_type, basic_environment<> >
            >::type
        nullary_result;

        actor& operator=(actor const& rhs)
        {
            Eval::operator=(rhs);
            return *this;
        }

        actor& operator=(actor& rhs)
        {
            Eval::operator=(rhs);
            return *this;
        }

        nullary_result
        operator()() const
        {
            return eval_type::eval(basic_environment<>());
        }

        template <class F, class A0>
        struct result<F(A0)>
          : eval_result<
                eval_type
              , basic_environment<
                    typename remove_reference<A0>::type
                >
            >
        {};

        template <typename T0>
        typename result<actor(T0&)>::type
        operator()(T0& _0) const
        {
            return eval_type::eval(basic_environment<T0>(_0));
        }

        template <class F, class A0, class A1>
        struct result<F(A0,A1)>
          : eval_result<
                eval_type
              , basic_environment<
                    typename remove_reference<A0>::type
                  , typename remove_reference<A1>::type
                >
            >
        {};

        template <typename T0, typename T1>
        typename result<actor(T0&,T1&)>::type
        operator()(T0& _0, T1& _1) const
        {
            return eval_type::eval(basic_environment<T0, T1>(_0, _1));
        }

        template <typename T1>
        typename detail::make_assign_composite<self_type, T1>::type
        operator=(T1 const& a1) const;

        template <typename T1>
        typename detail::make_index_composite<self_type, T1>::type
        operator[](T1 const& a1) const;

        //  Bring in the rest of the constructors and function call operators
        #include <boost/spirit/home/phoenix/core/detail/actor.hpp>
    };

#if defined(BOOST_MSVC)
# pragma warning(pop)
#endif

    //  Forward declaration: The intent to overload the comma must be
    //  stated early on to avoid the subtle problem that arises when
    //  the header file where the comma operator overload is defined,
    //  is not included by the client and the client attempts to use
    //  the comma anyway.

    namespace detail
    {
        template <typename BaseT0, typename BaseT1>
        struct comma_result;
    }

    template <typename BaseT0, typename BaseT1>
    typename detail::comma_result<BaseT0, BaseT1>::type
    operator,(actor<BaseT0> const& a0, actor<BaseT1> const& a1);
}}

namespace boost
{
    template <typename Eval>
    struct result_of<phoenix::actor<Eval>()>
    {
        typedef typename phoenix::actor<Eval>::nullary_result type;
    };

    template <typename Eval>
    struct result_of<phoenix::actor<Eval> const()>
        : result_of<phoenix::actor<Eval>()>
    {};
}

#endif
