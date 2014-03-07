/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_DISTANCE_IMPL_20060223_0814)
#define BOOST_FUSION_DISTANCE_IMPL_20060223_0814

#include <boost/mpl/minus.hpp>

namespace example
{
    struct example_struct_iterator_tag;
}

namespace boost { namespace fusion {

    namespace extension
    {
        template<typename Tag>
        struct distance_impl;

        template<>
        struct distance_impl<example::example_struct_iterator_tag>
        {
            template<typename First, typename Last>
            struct apply
                : mpl::minus<typename Last::index, typename First::index>
            {
                typedef apply<First, Last> self;

                static typename self::type
                call(First const& first, Last const& last)
                {
                    return typename self::type();
                }
            };
        };
    }
}}

#endif
