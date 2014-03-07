/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_CORE_ARGUMENT_HPP
#define PHOENIX_CORE_ARGUMENT_HPP

#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/preprocessor/inc.hpp>
#include <boost/spirit/home/phoenix/core/actor.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/less.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/size.hpp>
#include <boost/type_traits/add_reference.hpp>

#define PHOENIX_DECLARE_ARG(z, n, data)                                       \
    typedef actor<argument<n> >                                               \
        BOOST_PP_CAT(BOOST_PP_CAT(arg, BOOST_PP_INC(n)), _type);              \
    actor<argument<n> > const                                                 \
        BOOST_PP_CAT(arg, BOOST_PP_INC(n)) = argument<n>();                   \
    typedef actor<argument<n> >                                               \
        BOOST_PP_CAT(BOOST_PP_CAT(_, BOOST_PP_INC(n)), _type);                \
    actor<argument<n> > const                                                 \
        BOOST_PP_CAT(_, BOOST_PP_INC(n)) = argument<n>();

namespace boost { namespace phoenix
{
    namespace detail
    {
        template <typename Arg>
        struct error_argument_not_found {};
        inline void test_invalid_argument(int) {}
    }

    template <int N>
    struct argument
    {
        typedef mpl::true_ no_nullary;

        template <typename Env>
        struct result
        {
            typedef typename
                fusion::result_of::at<typename Env::tie_type, mpl::int_<N> >::type
            type;
        };

        template <typename Env>
        typename result<Env>::type
        eval(Env const& env) const
        {
            typedef typename
                mpl::if_<
                    mpl::less<mpl::int_<N>, mpl::size<typename Env::args_type> >
                  , int
                  , detail::error_argument_not_found<argument<N> >
                >::type
            check_out_of_bounds;

            detail::test_invalid_argument(check_out_of_bounds());
            return fusion::at_c<N>(env.args());
        }
    };

    namespace arg_names
    {
    //  Phoenix style names
        typedef actor<argument<0> > arg1_type;
        actor<argument<0> > const arg1 = argument<0>();
        typedef actor<argument<1> > arg2_type;
        actor<argument<1> > const arg2 = argument<1>();
        typedef actor<argument<2> > arg3_type;
        actor<argument<2> > const arg3 = argument<2>();

    //  BLL style names
        typedef actor<argument<0> > _1_type;
        actor<argument<0> > const _1 = argument<0>();
        typedef actor<argument<1> > _2_type;
        actor<argument<1> > const _2 = argument<1>();
        typedef actor<argument<2> > _3_type;
        actor<argument<2> > const _3 = argument<2>();

    //  Bring in the rest or the Phoenix style arguments (arg4 .. argN+1)
    //  and BLL style arguments (_4 .. _N+1), using PP
        BOOST_PP_REPEAT_FROM_TO(
            3, PHOENIX_ARG_LIMIT, PHOENIX_DECLARE_ARG, _)
    }
}}

#undef PHOENIX_DECLARE_ARG
#endif
