/*=============================================================================
    Copyright (c) 2001-2013 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_MOVE_01192013_2225)
#define FUSION_MOVE_01192013_2225

#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/end.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/sequence/comparison/detail/equal_to.hpp>
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/config.hpp>
#include <boost/static_assert.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/ice.hpp>

#if defined (BOOST_MSVC)
#  pragma warning(push)
#  pragma warning (disable: 4100) // unreferenced formal parameter
#endif

namespace boost { namespace fusion
{
    namespace detail
    {
        template <typename Seq1, typename Seq2>
        struct sequence_move
        {
            typedef typename result_of::end<Seq1>::type end1_type;
            typedef typename result_of::end<Seq2>::type end2_type;

            template <typename I1, typename I2>
            static void
            call(I1 const&, I2 const&, mpl::true_)
            {
            }

            template <typename I1, typename I2>
            static void
            call(I1 const& src, I2 const& dest, mpl::false_)
            {
                *dest = std::move(*src);
                call(fusion::next(src), fusion::next(dest));
            }

            template <typename I1, typename I2>
            static void
            call(I1 const& src, I2 const& dest)
            {
                typename result_of::equal_to<I1, end1_type>::type eq;
                return call(src, dest, eq);
            }
        };
    }

    template <typename Seq1, typename Seq2>
    inline
    typename
        enable_if_c<
            type_traits::ice_and<
                traits::is_sequence<Seq1>::value
              , traits::is_sequence<Seq2>::value
            >::value,
            void
        >::type
    move(Seq1&& src, Seq2& dest)
    {
        BOOST_STATIC_ASSERT(
            result_of::size<Seq1>::value <= result_of::size<Seq2>::value);

        detail::sequence_move<
            Seq1, Seq2>::
            call(fusion::begin(src), fusion::begin(dest));
    }
}}

#if defined (BOOST_MSVC)
#  pragma warning(pop)
#endif

#endif
