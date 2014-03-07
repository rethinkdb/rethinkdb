/*=============================================================================
    Copyright (c) 2005-2012 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_DEQUE_DETAIL_CPP11_DEQUE_KEYED_VALUES_07042012_1901)
#define BOOST_FUSION_DEQUE_DETAIL_CPP11_DEQUE_KEYED_VALUES_07042012_1901

#include <boost/fusion/container/deque/detail/keyed_element.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/mpl/int.hpp>

namespace boost { namespace fusion { namespace detail
{
    template<typename Key, typename Value, typename Rest>
    struct keyed_element;

    template <typename N, typename ...Elements>
    struct deque_keyed_values_impl;

    template <typename N, typename Head, typename ...Tail>
    struct deque_keyed_values_impl<N, Head, Tail...>
    {
        typedef mpl::int_<(N::value + 1)> next_index;
        typedef typename deque_keyed_values_impl<next_index, Tail...>::type tail;
        typedef keyed_element<N, Head, tail> type;

        static type construct(
          typename detail::call_param<Head>::type head
        , typename detail::call_param<Tail>::type... tail)
        {
            return type(
                head
              , deque_keyed_values_impl<next_index, Tail...>::construct(tail...)
            );
        }

        template <typename Head_, typename ...Tail_>
        static type forward_(Head_&& head, Tail_&&... tail)
        {
            return type(
                std::forward<Head_>(head)
              , deque_keyed_values_impl<next_index, Tail_...>::
                  forward_(std::forward<Tail_>(tail)...)
            );
        }
    };

    struct nil_keyed_element;

    template <typename N>
    struct deque_keyed_values_impl<N>
    {
        typedef nil_keyed_element type;
        static type construct() { return type(); }
        static type forward_() { return type(); }
    };

    template <typename ...Elements>
    struct deque_keyed_values
      : deque_keyed_values_impl<mpl::int_<0>, Elements...> {};
}}}

#endif
