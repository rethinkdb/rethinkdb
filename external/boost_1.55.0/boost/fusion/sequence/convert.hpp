/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_CONVERT_10022005_1442)
#define FUSION_CONVERT_10022005_1442

namespace boost { namespace fusion
{
    namespace extension
    {
        template <typename Tag>
        struct convert_impl;
    }

    namespace result_of
    {
        template <typename Tag, typename Sequence>
        struct convert
        {
            typedef typename extension::convert_impl<Tag> gen;

            typedef typename
                gen::template apply<Sequence>::type
            type;
        };
    }

    template <typename Tag, typename Sequence>
    inline typename result_of::convert<Tag, Sequence>::type
    convert(Sequence& seq)
    {
        typedef typename result_of::convert<Tag, Sequence>::gen gen;
        return gen::call(seq);
    }

    template <typename Tag, typename Sequence>
    inline typename result_of::convert<Tag, Sequence const>::type
    convert(Sequence const& seq)
    {
        typedef typename result_of::convert<Tag, Sequence const>::gen gen;
        return gen::call(seq);
    }
}}

#endif
