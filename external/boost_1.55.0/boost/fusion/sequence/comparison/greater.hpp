/*=============================================================================
    Copyright (c) 1999-2003 Jaakko Jarvi
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_GREATER_05052005_0432)
#define FUSION_GREATER_05052005_0432

#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/end.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/sequence/comparison/enable_comparison.hpp>

#if defined(FUSION_DIRECT_OPERATOR_USAGE)
#include <boost/fusion/sequence/comparison/detail/greater.hpp>
#else
#include <boost/fusion/sequence/comparison/less.hpp>
#endif

namespace boost { namespace fusion
{
    template <typename Seq1, typename Seq2>
    inline bool
    greater(Seq1 const& a, Seq2 const& b)
    {
#if defined(FUSION_DIRECT_OPERATOR_USAGE)
        return detail::sequence_greater<Seq1 const, Seq2 const>::
            call(fusion::begin(a), fusion::begin(b));
#else
        return (b < a);
#endif
    }

    namespace operators
    {
        template <typename Seq1, typename Seq2>
        inline typename
            boost::enable_if<
                traits::enable_comparison<Seq1, Seq2>
              , bool
            >::type
        operator>(Seq1 const& a, Seq2 const& b)
        {
            return fusion::greater(a, b);
        }
    }
    using operators::operator>;
}}

#endif
