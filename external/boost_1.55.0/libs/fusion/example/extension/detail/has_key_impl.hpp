/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_HAS_KEY_IMPL_20060223_2156)
#define BOOST_FUSION_HAS_KEY_IMPL_20060223_2156

#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/or.hpp>

namespace fields
{
    struct name;
    struct age;
}

namespace example
{
    struct example_sequence_tag;
}

namespace boost { namespace fusion {

    namespace extension
    {
        template<typename Tag>
        struct has_key_impl;

        template<>
        struct has_key_impl<example::example_sequence_tag>
        {
            template<typename Sequence, typename Key>
            struct apply
                : mpl::or_<
                is_same<Key, fields::name>,
                is_same<Key, fields::age> >
            {};
        };
    }
}}

#endif
