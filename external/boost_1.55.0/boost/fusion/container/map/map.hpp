/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_MAP_MAIN_07212005_1106)
#define FUSION_MAP_MAIN_07212005_1106

#include <boost/fusion/container/map/map_fwd.hpp>
#include <boost/fusion/support/pair.hpp>

///////////////////////////////////////////////////////////////////////////////
// Without variadics, we will use the PP version
///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_FUSION_HAS_VARIADIC_MAP)
# include <boost/fusion/container/map/detail/cpp03/map.hpp>
#else

///////////////////////////////////////////////////////////////////////////////
// C++11 interface
///////////////////////////////////////////////////////////////////////////////
#include <boost/fusion/container/map/detail/map_impl.hpp>
#include <boost/fusion/container/map/detail/begin_impl.hpp>
#include <boost/fusion/container/map/detail/end_impl.hpp>
#include <boost/fusion/container/map/detail/at_impl.hpp>
#include <boost/fusion/container/map/detail/at_key_impl.hpp>
#include <boost/fusion/container/map/detail/value_at_impl.hpp>
#include <boost/fusion/container/map/detail/value_at_key_impl.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/end.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/sequence/intrinsic/at_c.hpp>
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/fusion/support/sequence_base.hpp>
#include <boost/fusion/support/category_of.hpp>

#include <boost/utility/enable_if.hpp>

namespace boost { namespace fusion
{
    struct map_tag;

    template <typename ...T>
    struct map : detail::map_impl<0, T...>, sequence_base<map<T...>>
    {
        typedef map_tag fusion_tag;
        typedef detail::map_impl<0, T...> base_type;

        struct category : random_access_traversal_tag, associative_tag {};
        typedef mpl::int_<base_type::size> size;
        typedef mpl::false_ is_view;

        map() {}

        map(map const& seq)
          : base_type(seq.base())
        {}

        map(map&& seq)
          : base_type(std::forward<map>(seq))
        {}

        template <typename Sequence>
        map(Sequence const& seq
          , typename enable_if<traits::is_sequence<Sequence>>::type* /*dummy*/ = 0)
          : base_type(begin(seq), detail::map_impl_from_iterator())
        {}

        template <typename Sequence>
        map(Sequence& seq
          , typename enable_if<traits::is_sequence<Sequence>>::type* /*dummy*/ = 0)
          : base_type(begin(seq), detail::map_impl_from_iterator())
        {}

        template <typename Sequence>
        map(Sequence&& seq
          , typename enable_if<traits::is_sequence<Sequence>>::type* /*dummy*/ = 0)
          : base_type(begin(seq), detail::map_impl_from_iterator())
        {}

        template <typename First, typename ...T_>
        map(First const& first, T_ const&... rest)
          : base_type(first, rest...)
        {}

        template <typename First, typename ...T_>
        map(First&& first, T_&&... rest)
          : base_type(std::forward<First>(first), std::forward<T_>(rest)...)
        {}

        map& operator=(map const& rhs)
        {
            base_type::operator=(rhs.base());
            return *this;
        }

        map& operator=(map&& rhs)
        {
            base_type::operator=(std::forward<base_type>(rhs.base()));
            return *this;
        }

        template <typename Sequence>
        typename enable_if<traits::is_sequence<Sequence>, map&>::type
        operator=(Sequence const& seq)
        {
            base().assign(begin(seq), detail::map_impl_from_iterator());
            return *this;
        }

        base_type& base() { return *this; }
        base_type const& base() const { return *this; }
    };
}}

#endif
#endif
