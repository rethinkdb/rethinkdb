/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_CONVERT_MAIN_09232005_1340)
#define FUSION_CONVERT_MAIN_09232005_1340

#include <boost/fusion/container/map/map.hpp>

///////////////////////////////////////////////////////////////////////////////
// Without variadics, we will use the PP version
///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_FUSION_HAS_VARIADIC_MAP)
# include <boost/fusion/container/map/detail/cpp03/convert.hpp>

#else
///////////////////////////////////////////////////////////////////////////////
// C++11 variadic implementation
///////////////////////////////////////////////////////////////////////////////
#include <boost/fusion/container/map/detail/build_map.hpp>

namespace boost { namespace fusion
{
    namespace result_of
    {
        template <typename Sequence>
        struct as_map :
            detail::build_map<
                typename result_of::begin<Sequence>::type
              , typename result_of::end<Sequence>::type
            >
        {
        };
    }

    template <typename Sequence>
    inline typename result_of::as_map<Sequence>::type
    as_map(Sequence& seq)
    {
        typedef result_of::as_map<Sequence> gen;
        return gen::call(fusion::begin(seq), fusion::end(seq));
    }

    template <typename Sequence>
    inline typename result_of::as_map<Sequence const>::type
    as_map(Sequence const& seq)
    {
        typedef result_of::as_map<Sequence const> gen;
        return gen::call(fusion::begin(seq), fusion::end(seq));
    }

    namespace extension
    {
        template <typename T>
        struct convert_impl;

        template <>
        struct convert_impl<map_tag>
        {
            template <typename Sequence>
            struct apply
            {
                typedef typename
                    result_of::as_map<Sequence>::type
                type;

                static type call(Sequence& seq)
                {
                    typedef result_of::as_map<Sequence> gen;
                    return gen::call(fusion::begin(seq), fusion::end(seq));
                }
            };
        };
    }
}}

#endif
#endif

