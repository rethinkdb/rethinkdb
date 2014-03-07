/*=============================================================================
    Copyright (c) 2005-2007 Dan Marsden
    Copyright (c) 2005-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef PHOENIX_STATEMENT_TRY_CATCH_HPP
#define PHOENIX_STATEMENT_TRY_CATCH_HPP

#include <boost/spirit/home/phoenix/core/actor.hpp>
#include <boost/spirit/home/phoenix/core/composite.hpp>

#include <boost/fusion/include/push_back.hpp>
#include <boost/fusion/include/as_vector.hpp>

#include <boost/spirit/home/phoenix/statement/detail/catch_composite.hpp>
#include <boost/spirit/home/phoenix/statement/detail/catch_eval.hpp>
#include <boost/spirit/home/phoenix/statement/detail/catch_all_eval.hpp>

#if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4355)
#endif

namespace boost { namespace phoenix {

    template<typename Tuple> struct try_catch_composite;

    namespace meta
    {
        template<typename Composite, typename Actor>
        struct try_catch_composite_push_back
        {
            typedef typename Composite::base_type actor_tuple;
            typedef try_catch_composite<
                typename fusion::result_of::as_vector<
                typename fusion::result_of::push_back<
                actor_tuple, Actor>::type>::type> type;
        };

        template<typename Composite, typename Actor>
        struct catch_all_composite_push_back
        {
            typedef typename Composite::base_type actor_tuple;

            typedef composite<
                catch_all_eval,
                typename fusion::result_of::as_vector<
                typename fusion::result_of::push_back<
                actor_tuple, Actor>::type>::type> type;
        };
    }

    namespace detail
    {
        struct try_catch_composite_push_back
        {
            template<typename Composite, typename Actor>
            struct result
                : meta::try_catch_composite_push_back<Composite, Actor>
            {};

            template<typename Composite, typename Actor>
            typename result<Composite, Actor>::type
            operator()(
                const Composite& composite, const Actor& actor) const
            {
                typedef typename result<Composite, Actor>::type result;
                return result(
                    fusion::as_vector(
                        fusion::push_back(composite, actor)));
            }
        };

        struct catch_all_composite_push_back
        {
            template<typename Composite, typename Actor>
            struct result
                : meta::catch_all_composite_push_back<Composite, Actor>
            {};

            template<typename Composite, typename Actor>
            typename result<Composite, Actor>::type
            operator()(
                const Composite& composite, const Actor& actor) const
            {
                typedef typename result<Composite, Actor>::type result;
                return result(
                    fusion::as_vector(
                        fusion::push_back(composite, actor)));
            }
        };

    }

    detail::try_catch_composite_push_back const try_catch_composite_push_back
        = detail::try_catch_composite_push_back();
    detail::catch_all_composite_push_back const catch_all_composite_push_back
        = detail::catch_all_composite_push_back();

    template<typename Exception, typename SourceComposite>
    struct catch_gen
    {
        explicit catch_gen(
            const SourceComposite& sourceComposite)
            : mSourceComposite(sourceComposite) { }

        template<typename Actor>
        actor<typename meta::try_catch_composite_push_back<
            SourceComposite,
            detail::catch_composite<Exception, Actor> >::type>
        operator[](const Actor& actor) const
        {
            return try_catch_composite_push_back(
                mSourceComposite, detail::catch_composite<Exception, Actor>(actor));
        }

        const SourceComposite& mSourceComposite;
    };

    template<typename SourceComposite>
    struct catch_all_gen
    {
        explicit catch_all_gen(
            const SourceComposite& sourceComposite)
            : mSourceComposite(sourceComposite) { }

        template<typename Actor>
        actor<typename meta::catch_all_composite_push_back<SourceComposite, Actor>::type>
        operator[](const Actor& actor) const
        {
            return catch_all_composite_push_back(
                mSourceComposite, actor);
        }

        const SourceComposite& mSourceComposite;
    };

    template<typename Tuple>
    struct try_catch_composite
        : composite<catch_eval, Tuple>
    {
        explicit try_catch_composite(
            const Tuple& t)
            :
            composite<catch_eval, Tuple>(t),
            catch_all(*this) { }

        try_catch_composite(
            const try_catch_composite& rhs)
            : composite<catch_eval, Tuple>(rhs),
              catch_all(*this) { }

        template<typename Exception>
        catch_gen<Exception, try_catch_composite> catch_() const
        {
            return catch_gen<Exception, try_catch_composite>(
                *this);
        }

        const catch_all_gen<try_catch_composite> catch_all;

    private:
        try_catch_composite& operator=(const try_catch_composite&);
    };

    struct try_gen
    {
        template<typename Try>
        try_catch_composite<fusion::vector<Try> > operator[](
            const Try& try_) const
        {
            typedef fusion::vector<Try> tuple_type;
            return try_catch_composite<tuple_type>(
                tuple_type(try_));
        }
    };

    try_gen const try_ = try_gen();
}}

#if defined(BOOST_MSVC)
# pragma warning(pop)
#endif

#endif
