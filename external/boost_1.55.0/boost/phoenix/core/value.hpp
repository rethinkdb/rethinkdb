/*==============================================================================
    Copyright (c) 2001-2010 Joel de Guzman
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PHOENIX_CORE_VALUE_HPP
#define BOOST_PHOENIX_CORE_VALUE_HPP

#include <boost/phoenix/core/limits.hpp>
#include <boost/phoenix/core/actor.hpp>
#include <boost/phoenix/core/as_actor.hpp>
#include <boost/phoenix/core/terminal.hpp>
#include <boost/utility/result_of.hpp>

namespace boost { namespace phoenix
{
    ////////////////////////////////////////////////////////////////////////////
    //
    // values
    //
    //      function for evaluating values, e.g. val(123)
    //
    ////////////////////////////////////////////////////////////////////////////
 
    namespace expression
    {
        template <typename T>
        struct value
            : expression::terminal<T>
        {};
    }

    template <typename T>
    typename expression::value<T>::type const
    inline val(T t)
    {
        return expression::value<T>::make(t);
    }

    // Call out actor for special handling
    template<typename Expr>
    struct is_custom_terminal<actor<Expr> >
      : mpl::true_
    {};
    
    // Special handling for actor
    template<typename Expr>
    struct custom_terminal<actor<Expr> >
    {
        template <typename Sig>
        struct result;

        template <typename This, typename Actor, typename Context>
        struct result<This(Actor, Context)>
            : boost::remove_const<
				    typename boost::remove_reference<
                    typename evaluator::impl<Actor, Context, proto::empty_env>::result_type
                >::type
				>
        {};     

        template <typename Context>
        typename result<custom_terminal(actor<Expr> const &, Context &)>::type
        operator()(actor<Expr> const & expr, Context & ctx) const
        {
            return boost::phoenix::eval(expr, ctx);
        }
    };

    namespace meta
    {
        template<typename T>
        struct const_ref
            : add_reference<typename add_const<T>::type>
        {};

        template<typename T>
        struct argument_type
            : mpl::eval_if_c<
                is_function<typename remove_pointer<T>::type>::value
              , mpl::identity<T>
              , const_ref<T>
            >
        {
            typedef T type;
        };

        template <typename T>
        struct decay
        {
            typedef T type;
        };
        template <typename T, int N>
        struct decay<T[N]> : decay<T const *> {};
    }
    
    template <typename T>
    struct as_actor<T, mpl::false_>
    {
        typedef typename expression::value<typename meta::decay<T>::type >::type type;

        static type
        convert(typename meta::argument_type<typename meta::decay<T>::type>::type t)
        {
            return expression::value<typename meta::decay<T>::type >::make(t);
        }
    };
}}

#endif
