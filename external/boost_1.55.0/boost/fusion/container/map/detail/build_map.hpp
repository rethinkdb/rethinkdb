/*=============================================================================
    Copyright (c) 2005-2013 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_BUILD_MAP_02042013_1448)
#define BOOST_FUSION_BUILD_MAP_02042013_1448

#include <boost/fusion/iterator/equal_to.hpp>
#include <boost/fusion/iterator/next.hpp>
#include <boost/fusion/iterator/value_of.hpp>
#include <boost/fusion/iterator/deref.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/end.hpp>
#include <boost/fusion/container/map/map.hpp>
#include <boost/fusion/algorithm/transformation/push_front.hpp>

namespace boost { namespace fusion { namespace detail
{
    template <typename First, typename Last
      , bool is_empty = result_of::equal_to<First, Last>::value>
    struct build_map;

    template <typename First, typename Last>
    struct build_map<First, Last, true>
    {
        typedef map<> type;
        static type
        call(First const&, Last const&)
        {
            return type();
        }
    };

    template <typename T, typename Rest>
    struct push_front_map;

    template <typename T, typename ...Rest>
    struct push_front_map<T, map<Rest...>>
    {
        typedef map<T, Rest...> type;

        static type
        call(T const& first, map<Rest...> const& rest)
        {
            return type(push_front(rest, first));
        }
    };

    template <typename First, typename Last>
    struct build_map<First, Last, false>
    {
        typedef
            build_map<typename result_of::next<First>::type, Last>
        next_build_map;

        typedef push_front_map<
            typename result_of::value_of<First>::type
          , typename next_build_map::type>
        push_front;

        typedef typename push_front::type type;

        static type
        call(First const& f, Last const& l)
        {
            typename result_of::value_of<First>::type v = *f;
            return push_front::call(
                v, next_build_map::call(fusion::next(f), l));
        }
    };
}}}

#endif
