/*==============================================================================
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PHOENIX_CORE_FUNCTION_EQUAL_HPP
#define BOOST_PHOENIX_CORE_FUNCTION_EQUAL_HPP

#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/phoenix/core/limits.hpp>
#include <boost/is_placeholder.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/phoenix/core/terminal.hpp>
#include <boost/proto/matches.hpp>

namespace boost
{
    template <typename> class weak_ptr;
}

namespace boost { namespace phoenix
{
    template <typename>
    struct actor;

    namespace detail
    {
        struct compare
            : proto::callable
        {
            typedef bool result_type;

            template <typename A0, typename A1>
            result_type operator()(A0 const & a0, A1 const & a1) const
            {
                return a0 == a1;
            }
            
            // hard wiring reference_wrapper and weak_ptr here ...
            // **TODO** find out why boost bind does this ...
            template <typename A0, typename A1>
            result_type
            operator()(
                reference_wrapper<A0> const & a0
              , reference_wrapper<A1> const & a1
            ) const
            {
                return a0.get_pointer() == a1.get_pointer();
            }

            template <typename A0, typename A1>
            result_type
            operator()(weak_ptr<A0> const & a0, weak_ptr<A1> const & a1) const
            {
                return !(a0 < a1) && !(a1 < a0);
            }
        };

        struct function_equal_otherwise;

        struct function_equal_
            : proto::when<
                proto::if_<
                    proto::matches<proto::_, proto::_state>()
                  , proto::or_<
                        proto::when<
                            proto::terminal<proto::_>
                          , compare(
                                proto::_value
                              , proto::call<
                                    proto::_value(proto::_state)
                                >
                            )
                        >
                      , proto::otherwise<function_equal_otherwise(proto::_, proto::_state)>
                    >
                  , proto::call<function_equal_otherwise()>
                >
            >
        {};

        struct function_equal_otherwise
            : proto::callable
        {
            typedef bool result_type;

            result_type operator()() const
            {
                return false;
            }

            template <typename Expr1>
            result_type operator()(Expr1 const& e1, Expr1 const& e2) const
            {
                return
                    this->evaluate(
                        e1
                      , e2
                      , typename proto::arity_of<Expr1>::type()
                    );
            }

            private:
#if !defined(BOOST_PHOENIX_DONT_USE_PREPROCESSED_FILES)
#include <boost/phoenix/core/preprocessed/function_equal.hpp>
#else
#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(preserve: 2, line: 0, output: "preprocessed/function_equal_" BOOST_PHOENIX_LIMIT_STR ".hpp")
#endif

                #define BOOST_PHOENIX_FUNCTION_EQUAL_R(Z, N, DATA)              \
                    && function_equal_()(                                       \
                            proto::child_c< N >(e1)                             \
                          , proto::child_c< N >(e2)                             \
                        )                                                       \
                /**/

                #define BOOST_PHOENIX_FUNCTION_EQUAL(Z, N, DATA)                \
                    template <typename Expr1>                                   \
                    result_type                                                 \
                    evaluate(                                                   \
                        Expr1 const& e1                                         \
                      , Expr1 const& e2                                         \
                      , mpl::long_< N >                                         \
                    ) const                                                     \
                    {                                                           \
                        return                                                  \
                            function_equal_()(                                  \
                                proto::child_c<0>(e1)                           \
                              , proto::child_c<0>(e2)                           \
                            )                                                   \
                            BOOST_PP_REPEAT_FROM_TO(                            \
                                1                                               \
                              , N                                               \
                              , BOOST_PHOENIX_FUNCTION_EQUAL_R                  \
                              , _                                               \
                            );                                                  \
                    }                                                           \
                /**/

                BOOST_PP_REPEAT_FROM_TO(
                    1
                  , BOOST_PP_INC(BOOST_PROTO_MAX_ARITY)
                  , BOOST_PHOENIX_FUNCTION_EQUAL
                  , _
                )
                #undef BOOST_PHOENIX_FUNCTION_EQUAL_R
                #undef BOOST_PHOENIX_FUNCTION_EQUAL

#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(output: null)
#endif
#endif
        };
    }

    template <typename Expr1, typename Expr2>
    inline bool function_equal_impl(actor<Expr1> const& a1, actor<Expr2> const& a2)
    {
        return detail::function_equal_()(a1, a2);
    }

    template <typename Expr1, typename Expr2>
    inline bool function_equal(actor<Expr1> const& a1, actor<Expr2> const& a2)
    {
        return function_equal_impl(a1, a2);
    }

}}

#endif

