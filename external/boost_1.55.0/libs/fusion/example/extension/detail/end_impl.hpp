/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_END_IMPL_20060222_2042)
#define BOOST_FUSION_END_IMPL_20060222_2042

#include "../example_struct_iterator.hpp"

namespace example
{
    struct example_sequence_tag;
}

namespace boost { namespace fusion {
    
    namespace extension
    {
        template<typename Tag>
        struct end_impl;

        template<>
        struct end_impl<example::example_sequence_tag>
        {
            template<typename Sequence>
            struct apply
            {
                typedef example::example_struct_iterator<Sequence, 2> type;

                static type
                call(Sequence& seq)
                {
                    return type(seq);
                }
            };
        };
    }
}}

#endif
