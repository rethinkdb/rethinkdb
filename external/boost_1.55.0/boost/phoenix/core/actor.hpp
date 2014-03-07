/*=============================================================================
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010 Eric Niebler
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PHOENIX_CORE_ACTOR_HPP
#define BOOST_PHOENIX_CORE_ACTOR_HPP

#include <boost/phoenix/core/limits.hpp>

#include <boost/is_placeholder.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/phoenix/core/domain.hpp>
#include <boost/phoenix/core/environment.hpp>
#include <boost/phoenix/core/is_nullary.hpp>
#include <boost/phoenix/core/meta_grammar.hpp>
#include <boost/phoenix/support/iterate.hpp>
#include <boost/phoenix/support/vector.hpp>
#include <boost/proto/extends.hpp>
#include <boost/proto/make_expr.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/mpl/void.hpp>
#include <cstring>

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable: 4522) // 'this' used in base member initializer list
#endif

namespace boost { namespace phoenix
{
    template <typename Expr>
    struct actor;

    namespace detail
    {
        struct error_expecting_arguments
        {
            template <typename T>
            error_expecting_arguments(T const&) {}
        };
        
        struct error_invalid_lambda_expr
        {
            template <typename T>
            error_invalid_lambda_expr(T const&) {}
        };

        template <typename T>
        struct result_type_deduction_helper
        {
            typedef T const & type;
        };

        template <typename T>
        struct result_type_deduction_helper<T &>
        {
            typedef T & type;
        };

        template <typename T>
        struct result_type_deduction_helper<T const &>
        {
            typedef T const & type;
        };

        struct do_assign
        {
            BOOST_PROTO_CALLABLE()

            typedef void result_type;

            template <typename T1, typename T2>
            void operator()(T1 & t1, T2 const & t2) const
            {
                proto::value(t1) = proto::value(t2);
            }
        };

    #define BOOST_PHOENIX_ACTOR_ASSIGN_CHILD(Z, N, D)                           \
        assign(                                                                 \
            proto::_child_c<N>                                                  \
          , proto::call<                                                        \
                proto::_child_c<N>(proto::_state)                               \
            >                                                                   \
        )                                                                       \
    /**/
    #define BOOST_PHOENIX_ACTOR_ASSIGN_CALL(Z, N, D)                            \
            proto::when<                                                        \
                proto::nary_expr<proto::_ ,                                     \
                  BOOST_PP_ENUM_PARAMS(N, proto::_ BOOST_PP_INTERCEPT)          \
                >                                                               \
               , proto::and_<                                                   \
                  BOOST_PP_ENUM(                                                \
                        N                                                       \
                      , BOOST_PHOENIX_ACTOR_ASSIGN_CHILD                        \
                      , _                                                       \
                    )                                                           \
                >                                                               \
            >                                                                   \
      /**/

#if !defined(BOOST_PHOENIX_DONT_USE_PREPROCESSED_FILES)
#include <boost/phoenix/core/preprocessed/actor.hpp>
#else
#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(preserve: 2, line: 0, output: "preprocessed/actor_" BOOST_PHOENIX_LIMIT_STR ".hpp")
#endif
/*==============================================================================
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010-2011 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(preserve: 1)
#endif

        struct assign
            : proto::or_<
                BOOST_PP_ENUM_SHIFTED(
                    BOOST_PHOENIX_LIMIT
                  , BOOST_PHOENIX_ACTOR_ASSIGN_CALL
                  , _
                )
              , proto::when<
                    proto::terminal<proto::_>
                  , do_assign(proto::_, proto::_state)
                >
            >
        {};

#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(output: null)
#endif

#endif
    #undef BOOST_PHOENIX_ACTOR_ASSIGN_CALL
    #undef BOOST_PHOENIX_ACTOR_ASSIGN_CHILD
    }

    // Bring in the result_of::actor<>
    #include <boost/phoenix/core/detail/actor_result_of.hpp>

    ////////////////////////////////////////////////////////////////////////////
    //
    //  actor
    //
    //      The actor class. The main thing! In phoenix, everything is an actor
    //      This class is responsible for full function evaluation. Partial
    //      function evaluation involves creating a hierarchy of actor objects.
    //
    ////////////////////////////////////////////////////////////////////////////
    template <typename Expr>
    struct actor
    {
        typedef typename
            mpl::eval_if_c<
                mpl::or_<
                    is_custom_terminal<Expr>
                  , mpl::bool_<is_placeholder<Expr>::value>
                >::value
              , proto::terminal<Expr>
              , mpl::identity<Expr>
            >::type
            expr_type;
        
        BOOST_PROTO_BASIC_EXTENDS(expr_type, actor<expr_type>, phoenix_domain)

        // providing operator= to be assignable
        actor& operator=(actor const& other)
        {
            detail::assign()(*this, other);
            return *this;
        }
        actor& operator=(actor & other)
        {
            detail::assign()(*this, other);
            return *this;
        }

        template <typename A0>
        typename proto::result_of::make_expr<
            proto::tag::assign
          , phoenix_domain
          , proto_base_expr
          , A0
        >::type const
        operator=(A0 const & a0) const
        {
            return proto::make_expr<proto::tag::assign, phoenix_domain>(this->proto_expr_, a0);
        }

        template <typename A0>
        typename proto::result_of::make_expr<
            proto::tag::assign
          , phoenix_domain
          , proto_base_expr
          , A0
        >::type const
        operator=(A0 & a0) const
        {
            return proto::make_expr<proto::tag::assign, phoenix_domain>(this->proto_expr_, a0);
        }
        
        template <typename A0>
        typename proto::result_of::make_expr<
            proto::tag::subscript
          , phoenix_domain
          , proto_base_expr
          , A0
        >::type const
        operator[](A0 const & a0) const
        {
            return proto::make_expr<proto::tag::subscript, phoenix_domain>(this->proto_expr_, a0);
        }

        template <typename A0>
        typename proto::result_of::make_expr<
            proto::tag::subscript
          , phoenix_domain
          , proto_base_expr
          , A0
        >::type const
        operator[](A0 & a0) const
        {
            return proto::make_expr<proto::tag::subscript, phoenix_domain>(this->proto_expr_, a0);
        }

        template <typename Sig>
        struct result;

        typename result_of::actor<proto_base_expr>::type
        operator()()
        {
            typedef vector1<const actor<Expr> *> env_type;
            env_type env = {this};
            
            return phoenix::eval(*this, phoenix::context(env, default_actions()));
        }

        typename result_of::actor<proto_base_expr>::type
        operator()() const
        {
            typedef vector1<const actor<Expr> *> env_type;
            env_type env = {this};
            
            return phoenix::eval(*this, phoenix::context(env, default_actions()));
        }

        template <typename Env>
        typename evaluator::impl<
            proto_base_expr const &
          , typename result_of::context<
                Env const &
              , default_actions const &
            >::type
          , proto::empty_env
        >::result_type
        eval(Env const & env) const
        {
            return phoenix::eval(*this, phoenix::context(env, default_actions()));
        }
        
        // Bring in the rest
        #include <boost/phoenix/core/detail/actor_operator.hpp>
    };

}}

namespace boost
{
    // specialize boost::result_of to return the proper result type
    template <typename Expr>
    struct result_of<phoenix::actor<Expr>()>
        : phoenix::result_of::actor<typename phoenix::actor<Expr>::proto_base_expr>
    {};
    
    template <typename Expr>
    struct result_of<phoenix::actor<Expr> const()>
        : phoenix::result_of::actor<typename phoenix::actor<Expr>::proto_base_expr>
    {};
}


#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif

