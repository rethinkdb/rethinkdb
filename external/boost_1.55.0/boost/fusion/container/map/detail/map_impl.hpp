/*=============================================================================
    Copyright (c) 2005-2013 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_MAP_IMPL_02032013_2233)
#define BOOST_FUSION_MAP_IMPL_02032013_2233

#include <boost/fusion/support/detail/access.hpp>
#include <boost/fusion/iterator/deref.hpp>
#include <boost/fusion/iterator/next.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/identity.hpp>

namespace boost { namespace fusion
{
    struct fusion_sequence_tag;
}}

namespace boost { namespace fusion { namespace detail
{
    struct map_impl_from_iterator {};

    template <int index, typename ...T>
    struct map_impl;

    template <int index_>
    struct map_impl<index_>
    {
        typedef fusion_sequence_tag tag;
        static int const index = index_;
        static int const size = 0;

        map_impl() {}

        template <typename Iterator>
        map_impl(Iterator const& iter, map_impl_from_iterator)
        {}

        template <typename Iterator>
        void assign(Iterator const& iter, map_impl_from_iterator)
        {}

        void get();
        void get_val();
        void get_key();
    };

    template <int index_, typename Pair, typename ...T>
    struct map_impl<index_, Pair, T...> : map_impl<index_ + 1, T...>
    {
        typedef fusion_sequence_tag tag;
        typedef map_impl<index_+1, T...> rest_type;

        using rest_type::get;
        using rest_type::get_val;
        using rest_type::get_key;

        static int const index = index_;
        static int const size = rest_type::size + 1;

        typedef Pair pair_type;
        typedef typename Pair::first_type key_type;
        typedef typename Pair::second_type value_type;

        map_impl()
          : rest_type(), element()
        {}

        map_impl(map_impl const& rhs)
          : rest_type(rhs.get_base()), element(rhs.element)
        {}

        map_impl(map_impl&& rhs)
          : rest_type(std::forward<rest_type>(*static_cast<rest_type*>(this)))
          , element(std::forward<Pair>(rhs.element))
        {}

        template <typename ...U>
        map_impl(map_impl<index, U...> const& rhs)
          : rest_type(rhs.get_base()), element(rhs.element)
        {}

        map_impl(typename detail::call_param<Pair>::type element
          , typename detail::call_param<T>::type... rest)
          : rest_type(rest...), element(element)
        {}

        map_impl(Pair&& element, T&&... rest)
          : rest_type(std::forward<T>(rest)...)
          , element(std::forward<Pair>(element))
        {}

        template <typename Iterator>
        map_impl(Iterator const& iter, map_impl_from_iterator fi)
          : rest_type(fusion::next(iter), fi)
          , element(*iter)
        {}

        rest_type& get_base()
        {
            return *this;
        }

        rest_type const& get_base() const
        {
            return *this;
        }

        value_type get_val(mpl::identity<key_type>);
        pair_type get_val(mpl::int_<index>);
        value_type get_val(mpl::identity<key_type>) const;
        pair_type get_val(mpl::int_<index>) const;

        key_type get_key(mpl::int_<index>);
        key_type get_key(mpl::int_<index>) const;

        typename cref_result<value_type>::type
        get(mpl::identity<key_type>) const
        {
            return element.second;
        }

        typename ref_result<value_type>::type
        get(mpl::identity<key_type>)
        {
            return element.second;
        }

        typename cref_result<pair_type>::type
        get(mpl::int_<index>) const
        {
            return element;
        }

        typename ref_result<pair_type>::type
        get(mpl::int_<index>)
        {
            return element;
        }

        template <typename ...U>
        map_impl& operator=(map_impl<index, U...> const& rhs)
        {
            rest_type::operator=(rhs);
            element = rhs.element;
            return *this;
        }

        map_impl& operator=(map_impl const& rhs)
        {
            rest_type::operator=(rhs);
            element = rhs.element;
            return *this;
        }

        map_impl& operator=(map_impl&& rhs)
        {
            rest_type::operator=(std::forward<map_impl>(rhs));
            element = std::forward<Pair>(rhs.element);
            return *this;
        }

        template <typename Iterator>
        void assign(Iterator const& iter, map_impl_from_iterator fi)
        {
            rest_type::assign(fusion::next(iter), fi);
            element = *iter;
        }

        Pair element;
    };
}}}

#endif
