/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_ITERATOR_ADAPTER_08112011_0942)
#define FUSION_ITERATOR_ADAPTER_08112011_0942

#include <boost/fusion/iterator/detail/advance.hpp>
#include <boost/fusion/iterator/iterator_facade.hpp>
#include <boost/type_traits/remove_const.hpp>

namespace boost { namespace fusion
{
    template <typename Derived_, typename Iterator_,
        typename Category = typename Iterator_::category>
    struct iterator_adapter
        : iterator_facade<Derived_, Category>
    {
        typedef typename
            remove_const<Iterator_>::type
        iterator_base_type;
        iterator_base_type iterator_base;

        iterator_adapter(iterator_base_type const& iterator_base_)
            : iterator_base(iterator_base_) {}

        // default implementation
        template <typename I1, typename I2>
        struct equal_to
            : result_of::equal_to<
                typename I1::iterator_base_type
              , typename I2::iterator_base_type
            >
        {};

        // default implementation
        template <typename Iterator, typename N>
        struct advance
        {
            typedef typename Derived_::template make<
                typename result_of::advance<
                    typename Iterator::iterator_base_type, N
                >::type>::type
            type;

            static type
            call(Iterator const& it)
            {
                return type(fusion::advance<N>(it.iterator_base));
            }
        };

        // default implementation
        template <typename First, typename Last>
        struct distance
            : result_of::distance<
                typename First::iterator_base_type
              , typename Last::iterator_base_type
            >
        {};

        // default implementation
        template <typename Iterator>
        struct value_of
            : result_of::value_of<
                typename Iterator::iterator_base_type
            >
        {};

        // default implementation
        template <typename Iterator>
        struct deref
        {
            typedef typename
                result_of::deref<
                    typename Iterator::iterator_base_type
                >::type
            type;

            static type
            call(Iterator const& it)
            {
                return fusion::deref(it.iterator_base);
            }
        };

        // default implementation
        template <typename Iterator>
        struct next
        {
            typedef typename Derived_::template make<
                typename result_of::next<
                    typename Iterator::iterator_base_type
                >::type>::type
            type;

            static type
            call(Iterator const& i)
            {
                return type(fusion::next(i.iterator_base));
            }
        };

        // default implementation
        template <typename Iterator>
        struct prior
        {
            typedef typename Derived_::template make<
                typename result_of::prior<
                    typename Iterator::iterator_base_type
                >::type>::type
            type;

            static type
            call(Iterator const& i)
            {
                return type(fusion::prior(i.iterator_base));
            }
        };
    };
}}

#endif
