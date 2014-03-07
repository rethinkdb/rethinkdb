/*=============================================================================
    Copyright (c) 2009 Christopher Schmidt

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_CONTAINER_SET_DETAIL_VALUE_OF_IMPL_HPP
#define BOOST_FUSION_CONTAINER_SET_DETAIL_VALUE_OF_IMPL_HPP

#include <boost/fusion/sequence/intrinsic/value_at.hpp>

namespace boost { namespace fusion { namespace extension
{
    template <typename>
    struct value_of_impl;

    template <>
    struct value_of_impl<set_iterator_tag>
    {
        template <typename It>
        struct apply
        {
            typedef typename
                result_of::value_at<
                    typename It::seq_type::storage_type
                  , typename It::index
                >::type
            type;
        };
    };
}}}

#endif
