/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_VALUE_AT_IMPL_20060223_2025)
#define BOOST_FUSION_VALUE_AT_IMPL_20060223_2025

namespace example
{
    struct example_sequence_tag;
}

namespace boost { namespace fusion {

    namespace extension
    {
        template<typename Tag>
        struct value_at_impl;

        template<>
        struct value_at_impl<example::example_sequence_tag>
        {
            template<typename Sequence, typename N>
            struct apply;

            template<typename Sequence>
            struct apply<Sequence, mpl::int_<0> >
            {
                typedef std::string type;
            };

            template<typename Sequence>
            struct apply<Sequence, mpl::int_<1> >
            {
                typedef int type;
            };
        };
    }
}}

#endif
