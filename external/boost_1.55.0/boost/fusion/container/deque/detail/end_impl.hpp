/*=============================================================================
    Copyright (c) 2005-2012 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_DEQUE_END_IMPL_09122006_2034)
#define BOOST_FUSION_DEQUE_END_IMPL_09122006_2034

#include <boost/fusion/container/deque/deque_iterator.hpp>

#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/if.hpp>

namespace boost { namespace fusion
{
    struct deque_tag;

    namespace extension
    {
        template<typename T>
        struct end_impl;

        template<>
        struct end_impl<deque_tag>
        {
            template<typename Sequence>
            struct apply
            {
                typedef typename
                    mpl::if_c<
                        (Sequence::next_down::value == Sequence::next_up::value)
                      , deque_iterator<Sequence, 0>
                      , deque_iterator<Sequence, Sequence::next_up::value>
                    >::type
                type;

                static type call(Sequence& seq)
                {
                    return type(seq);
                }
            };
        };
    }
}}

#endif
