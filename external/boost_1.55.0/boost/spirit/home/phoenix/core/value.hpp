/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_CORE_VALUE_HPP
#define PHOENIX_CORE_VALUE_HPP

#include <boost/spirit/home/phoenix/core/actor.hpp>
#include <boost/spirit/home/phoenix/core/as_actor.hpp>
#include <boost/static_assert.hpp>

#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/type_traits/is_function.hpp>

#include <boost/mpl/bool.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>

namespace boost { namespace phoenix
{
    namespace meta
    {
        template<typename T>
        struct const_ref
            : add_reference<typename add_const<T>::type>
        {};

        template<typename T>
        struct argument_type
            : mpl::eval_if<
            is_function<typename remove_pointer<T>::type>,
            mpl::identity<T>,
            const_ref<T> >
        {
            typedef T type;
        };
    }

    template <typename T>
    struct value
    {
        BOOST_STATIC_ASSERT(
            mpl::not_<is_reference<T> >::value != 0);

        typedef mpl::false_ no_nullary;

        template <typename Env>
        struct result
        {
            typedef T type;
        };

        value()
            : val() {}

        value(T const& arg)
            : val(arg) {}

        template <typename Env>
        T const&
        eval(Env const&) const
        {
            return val;
        }

        T val;
    };

    template <typename Actor>
    struct actor_value
    {
        typedef typename Actor::no_nullary no_nullary;

        template <typename Env>
        struct result
        {
            typedef typename
                remove_reference<
                    typename eval_result<Actor, Env>::type
                >::type
            type;
        };

        actor_value(Actor const& actor_)
            : actor(actor_) {}

        template <typename Env>
        typename result<Env>::type
        eval(Env const& env) const
        {
            return actor.eval(env);
        }

        Actor actor;
    };

    template <typename T>
    inline typename as_actor<T>::type
    val(T const& v)
    {
        return as_actor<T>::convert(v);
    }

    template <typename Derived>
    inline actor<actor_value<Derived> >
    val(actor<Derived> const& actor)
    {
        return actor_value<Derived>(actor);
    }

    template <typename T>
    struct as_actor_base
    {
        typedef value<T> type;

        static value<T>
        convert(typename meta::argument_type<T>::type x)
        {
            return value<T>(x);
        }
    };

    // Sometimes it is necessary to auto-convert references to
    // a value<T>. This happens when we are re-currying. This
    // cannot happen through the standard public actor interfaces.
    template <typename T>
    struct as_actor_base<T&>
    {
        typedef value<T> type;

        static value<T>
        convert(T& x)
        {
            return value<T>(x);
        }
    };

    template <typename T, int N>
    struct as_actor_base<T[N]>
    {
        typedef value<T const*> type;

        static value<T const*>
        convert(T const x[N])
        {
            return value<T const*>(x);
        }
    };
}}

#endif
