/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_STATEMENT_DETAIL_SWITCH_HPP
#define PHOENIX_STATEMENT_DETAIL_SWITCH_HPP

#include <boost/spirit/home/phoenix/core/nothing.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/as_vector.hpp>
#include <boost/fusion/include/push_back.hpp>
#include <boost/fusion/include/push_front.hpp>
#include <boost/fusion/include/begin.hpp>
#include <boost/fusion/include/size.hpp>
#include <boost/fusion/include/value_of.hpp>
#include <boost/fusion/include/is_sequence.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/if.hpp>

namespace boost { namespace phoenix
{

    template <typename Actor, typename K, K Value>
    struct switch_case;

    template <typename Actor>
    struct default_case;

    namespace detail
    {
        template <typename T>
        struct is_default_case : mpl::bool_<T::is_default> {};

        template <typename A0, typename A1>
        struct compose_case_a
        {
            // here, A0 and A1 are both switch cases
            typedef typename
                mpl::if_<
                    is_default_case<A1>
                  , fusion::vector<actor<A1>, actor<A0> >
                  , fusion::vector<actor<A0>, actor<A1> >
                >::type
            type;

            static type
            eval(A0 const& _0, A1 const& _1, mpl::false_)
            {
                return type(_0, _1);
            }

            static type
            eval(A0 const& _0, A1 const& _1, mpl::true_)
            {
                return type(_1, _0);
            }

            static type
            eval(A0 const& _0, A1 const& _1)
            {
                return eval(_0, _1, is_default_case<A1>());
            }
        };

        template <typename Seq, typename Case>
        struct compose_case_b
        {
            typedef typename fusion::result_of::as_vector<
                typename mpl::eval_if<
                    is_default_case<Case>
                  , fusion::result_of::push_front<Seq const, actor<Case> >
                  , fusion::result_of::push_back<Seq const, actor<Case> >
            >::type>::type
            type;

            static type
            eval(Seq const& seq, Case const& case_, mpl::false_)
            {
                return fusion::as_vector(
                    fusion::push_back(seq, actor<Case>(case_)));
            }

            static type
            eval(Seq const& seq, Case const& case_, mpl::true_)
            {
                return fusion::as_vector(
                    fusion::push_front(seq, actor<Case>(case_)));
            }

            static type
            eval(Seq const& seq, Case const& case_)
            {
                return eval(seq, case_, is_default_case<Case>());
            }
        };

        template <typename Cases>
        struct ensure_default
        {
            typedef
                is_default_case<
                    typename fusion::result_of::value_of<
                        typename fusion::result_of::begin<Cases>::type
                    >::type
                >
            is_default_case_;

            typedef typename
                mpl::eval_if<
                    is_default_case_
                  , mpl::identity<Cases>
                  , fusion::result_of::push_front<
                        Cases const, actor<default_case<actor<null_actor> > > >
                >::type
            type;

            static type
            eval(Cases const& cases, mpl::false_);

            static type
            eval(Cases const& cases, mpl::true_)
            {
                return cases;
            }

            static type
            eval(Cases const& cases)
            {
                return eval(cases, is_default_case_());
            }
        };

        template <typename Cond, typename Cases>
        struct switch_composite
        {
            BOOST_STATIC_ASSERT(fusion::traits::is_sequence<Cases>::value);
            typedef ensure_default<Cases> ensure_default_;

            typedef typename
                fusion::result_of::as_vector<
                    typename fusion::result_of::push_front<
                        typename ensure_default_::type, Cond>::type
                    >::type
            tuple_type;

            typedef
                composite<
                    detail::switch_eval<fusion::result_of::size<tuple_type>::value-2>
                  , tuple_type>
            type;

            static type
            eval(Cond const& cond, Cases const& cases)
            {
                return fusion::as_vector(
                    fusion::push_front(ensure_default_::eval(cases), cond));
            }
        };

        template <typename Cond, typename Cases>
        struct switch_composite_actor
        {
            typedef actor<typename switch_composite<Cond, Cases>::type> type;
        };
    }
}}

#endif
