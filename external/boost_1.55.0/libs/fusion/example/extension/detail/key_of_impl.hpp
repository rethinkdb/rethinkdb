/*=============================================================================
    Copyright (c) 2009 Christopher Schmidt

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_EXAMPLE_EXTENSION_DETAIL_KEY_OF_IMPL_HPP
#define BOOST_FUSION_EXAMPLE_EXTENSION_DETAIL_KEY_OF_IMPL_HPP

#include <boost/mpl/if.hpp>

namespace fields
{
    struct name;
    struct age;
}

namespace example
{
    struct example_struct_iterator_tag;
}

namespace boost { namespace fusion {

    namespace extension
    {
        template<typename Tag>
        struct key_of_impl;

        template<>
        struct key_of_impl<example::example_struct_iterator_tag>
        {
            template<typename It>
            struct apply
              : mpl::if_c<!It::index::value, fields::name, fields::age>
            {};
        };
    }
}}

#endif
