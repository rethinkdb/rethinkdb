/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_VALUE_OF_IMPL_20060223_1905)
#define BOOST_FUSION_VALUE_OF_IMPL_20060223_1905

#include <string>

namespace example
{
    struct example_struct_iterator_tag;

    template<typename Struct, int Pos>
    struct example_struct_iterator;
}

namespace boost { namespace fusion {

    namespace extension
    {
        template<typename Tag>
        struct value_of_impl;

        template<>
        struct value_of_impl<example::example_struct_iterator_tag>
        {
            template<typename Iterator>
            struct apply;

            template<typename Struct>
            struct apply<example::example_struct_iterator<Struct, 0> >
            {
                typedef std::string type;
            };

            template<typename Struct>
            struct apply<example::example_struct_iterator<Struct, 1> >
            {
                typedef int type;
            };
        };
    }
}}

#endif
