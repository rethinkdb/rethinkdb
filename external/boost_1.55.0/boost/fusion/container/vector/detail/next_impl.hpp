/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_NEXT_IMPL_05042005_1058)
#define FUSION_NEXT_IMPL_05042005_1058

#include <boost/fusion/container/vector/vector_iterator.hpp>

namespace boost { namespace fusion
{
    struct vector_iterator_tag;
    template <typename Vector, int N>
    struct vector_iterator;

    namespace extension
    {
        template <typename Tag>
        struct next_impl;

        template <>
        struct next_impl<vector_iterator_tag>
        {
            template <typename Iterator>
            struct apply 
            {
                typedef typename Iterator::vector vector;
                typedef typename Iterator::index index;
                typedef vector_iterator<vector, index::value+1> type;

                static type
                call(Iterator const& i)
                {
                    return type(i.vec);
                }
            };
        };
    }
}}

#endif
