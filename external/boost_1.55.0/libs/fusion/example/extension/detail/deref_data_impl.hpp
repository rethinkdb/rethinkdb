/*=============================================================================
    Copyright (c) 2009 Christopher Schmidt

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_EXAMPLE_EXTENSION_DETAIL_DEREF_DATA_IMPL_HPP
#define BOOST_FUSION_EXAMPLE_EXTENSION_DETAIL_DEREF_DATA_IMPL_HPP

namespace example
{
    struct example_struct_iterator_tag;
}

namespace boost { namespace fusion {

    namespace extension
    {
        template<typename Tag>
        struct deref_data_impl;

        template<>
        struct deref_data_impl<example::example_struct_iterator_tag>
          : deref_impl<example::example_struct_iterator_tag>
        {};
    }
}}

#endif
