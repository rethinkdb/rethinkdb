/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_AT_KEY_20060304_1755)
#define BOOST_FUSION_AT_KEY_20060304_1755

#include <boost/type_traits/is_const.hpp>
#include <boost/fusion/sequence/intrinsic_fwd.hpp>
#include <boost/fusion/algorithm/query/find.hpp>
#include <boost/fusion/iterator/deref_data.hpp>
#include <boost/fusion/support/tag_of.hpp>
#include <boost/fusion/support/detail/access.hpp>

namespace boost { namespace fusion
{
    // Special tags:
    struct sequence_facade_tag;
    struct boost_array_tag; // boost::array tag
    struct mpl_sequence_tag; // mpl sequence tag
    struct std_pair_tag; // std::pair tag

    namespace extension
    {
        template <typename Tag>
        struct at_key_impl
        {
            template <typename Seq, typename Key>
            struct apply
            {
                typedef typename
                    result_of::deref_data<
                        typename result_of::find<Seq, Key>::type
                    >::type
                type;

                static type
                call(Seq& seq)
                {
                    return fusion::deref_data(fusion::find<Key>(seq));
                }
            };
        };

        template <>
        struct at_key_impl<sequence_facade_tag>
        {
            template <typename Sequence, typename Key>
            struct apply : Sequence::template at_key_impl<Sequence, Key> {};
        };

        template <>
        struct at_key_impl<boost_array_tag>;

        template <>
        struct at_key_impl<mpl_sequence_tag>;

        template <>
        struct at_key_impl<std_pair_tag>;
    }

    namespace result_of
    {
        template <typename Sequence, typename Key>
        struct at_key
            : extension::at_key_impl<typename detail::tag_of<Sequence>::type>::
                template apply<Sequence, Key>
        {};
    }

    template <typename Key, typename Sequence>
    inline typename 
        lazy_disable_if<
            is_const<Sequence>
          , result_of::at_key<Sequence, Key>
        >::type
    at_key(Sequence& seq)
    {
        return result_of::at_key<Sequence, Key>::call(seq);
    }

    template <typename Key, typename Sequence>
    inline typename result_of::at_key<Sequence const, Key>::type
    at_key(Sequence const& seq)
    {
        return result_of::at_key<Sequence const, Key>::call(seq);
    }
}}

#endif
