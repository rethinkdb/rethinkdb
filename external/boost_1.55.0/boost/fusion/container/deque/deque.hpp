/*=============================================================================
    Copyright (c) 2005-2012 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_DEQUE_26112006_1649)
#define BOOST_FUSION_DEQUE_26112006_1649

# include <boost/fusion/container/deque/deque_fwd.hpp>

///////////////////////////////////////////////////////////////////////////////
// Without variadics, we will use the PP version
///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_FUSION_HAS_VARIADIC_DEQUE)
# include <boost/fusion/container/deque/detail/cpp03/deque.hpp>
#else

///////////////////////////////////////////////////////////////////////////////
// C++11 interface
///////////////////////////////////////////////////////////////////////////////
#include <boost/fusion/support/sequence_base.hpp>
#include <boost/fusion/support/detail/access.hpp>
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/fusion/container/deque/detail/keyed_element.hpp>
#include <boost/fusion/container/deque/detail/deque_keyed_values.hpp>
#include <boost/fusion/container/deque/deque_fwd.hpp>
#include <boost/fusion/container/deque/detail/value_at_impl.hpp>
#include <boost/fusion/container/deque/detail/at_impl.hpp>
#include <boost/fusion/container/deque/detail/begin_impl.hpp>
#include <boost/fusion/container/deque/detail/end_impl.hpp>
#include <boost/fusion/container/deque/detail/is_sequence_impl.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/empty.hpp>

#include <boost/mpl/int.hpp>
#include <boost/mpl/and.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_convertible.hpp>

namespace boost { namespace fusion
{
    struct deque_tag;

    template <typename ...Elements>
    struct deque : detail::nil_keyed_element
    {
        typedef deque_tag fusion_tag;
        typedef bidirectional_traversal_tag category;
        typedef mpl::int_<0> size;
        typedef mpl::int_<0> next_up;
        typedef mpl::int_<0> next_down;
        typedef mpl::false_ is_view;

        template <typename Sequence>
        deque(Sequence const&,
            typename enable_if<
                mpl::and_<
                    traits::is_sequence<Sequence>
                  , result_of::empty<Sequence>>>::type* /*dummy*/ = 0)
        {}

        deque() {}
    };

    template <typename Head, typename ...Tail>
    struct deque<Head, Tail...>
      : detail::deque_keyed_values<Head, Tail...>::type
      , sequence_base<deque<Head, Tail...>>
    {
        typedef deque_tag fusion_tag;
        typedef bidirectional_traversal_tag category;
        typedef typename detail::deque_keyed_values<Head, Tail...>::type base;
        typedef mpl::int_<(sizeof ...(Tail) + 1)> size;
        typedef mpl::int_<size::value> next_up;
        typedef mpl::int_<((size::value == 0) ? 0 : -1)> next_down;
        typedef mpl::false_ is_view;

        deque()
        {}

        template <typename ...Elements>
        deque(deque<Elements...> const& seq)
          : base(seq)
        {}

        template <typename ...Elements>
        deque(deque<Elements...>& seq)
          : base(seq)
        {}

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename ...Elements>
        deque(deque<Elements...>&& seq)
          : base(std::forward<deque<Elements...>>(seq))
        {}
#endif

        deque(deque const& seq)
          : base(seq)
        {}

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        deque(deque&& seq)
          : base(std::forward<deque>(seq))
        {}
#endif

        explicit deque(Head const& head, Tail const&... tail)
          : base(detail::deque_keyed_values<Head, Tail...>::construct(head, tail...))
        {}

        template <typename Head_, typename ...Tail_>
        explicit deque(Head_ const& head, Tail_ const&... tail)
          : base(detail::deque_keyed_values<Head_, Tail_...>::construct(head, tail...))
        {}

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename Head_, typename ...Tail_>
        explicit deque(Head_&& head, Tail_&&... tail)
          : base(detail::deque_keyed_values<Head, Tail...>
                ::forward_(std::forward<Head_>(head), std::forward<Tail_>(tail)...))
        {}
#endif

        template <typename Sequence>
        explicit deque(Sequence const& seq
          , typename disable_if<is_convertible<Sequence, Head> >::type* /*dummy*/ = 0)
          : base(base::from_iterator(fusion::begin(seq)))
        {}

        template <typename ...Elements>
        deque& operator=(deque<Elements...> const& rhs)
        {
            base::operator=(rhs);
            return *this;
        }

        template <typename T>
        deque& operator=(T const& rhs)
        {
            base::operator=(rhs);
            return *this;
        }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T>
        deque& operator=(T&& rhs)
        {
            base::operator=(std::forward<T>(rhs));
            return *this;
        }
#endif

    };
}}

#endif
#endif
