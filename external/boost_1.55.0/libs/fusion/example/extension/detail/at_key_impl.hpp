/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_AT_KEY_IMPL_20060223_2017)
#define BOOST_FUSION_AT_KEY_IMPL_20060223_2017

#include <string>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_const.hpp>

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
        struct at_key_impl;

        template<>
        struct at_key_impl<example::example_sequence_tag>
        {
            template<typename Sequence, typename Key>
            struct apply;

            template<typename Sequence>
            struct apply<Sequence, fields::name>
            {
                typedef typename mpl::if_<
                    is_const<Sequence>,
                    std::string const&,
                    std::string&>::type type;

                static type
                call(Sequence& seq)
                {
                    return seq.name;
                };
            };

            template<typename Sequence>
            struct apply<Sequence, fields::age>
            {
                typedef typename mpl::if_<
                    is_const<Sequence>,
                    int const&,
                    int&>::type type;

                static type
                call(Sequence& seq)
                {
                    return seq.age;
                };
            };
        };
    }
}}

#endif
