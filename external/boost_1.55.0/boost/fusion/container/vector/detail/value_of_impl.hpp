/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_VALUE_OF_IMPL_05052005_1128)
#define FUSION_VALUE_OF_IMPL_05052005_1128

#include <boost/mpl/at.hpp>

namespace boost { namespace fusion
{
    struct vector_iterator_tag;

    namespace extension
    {
        template <typename Tag>
        struct value_of_impl;

        template <>
        struct value_of_impl<vector_iterator_tag>
        {
            template <typename Iterator>
            struct apply 
            {
                typedef typename Iterator::vector vector;
                typedef typename Iterator::index index;
                typedef typename mpl::at<
                    typename vector::types, index>::type
                type;
            };
        };
    }
}}

#endif
