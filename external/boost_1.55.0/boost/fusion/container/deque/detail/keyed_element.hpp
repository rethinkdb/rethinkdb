/*=============================================================================
    Copyright (c) 2005-2012 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_DEQUE_DETAIL_KEYED_ELEMENT_26112006_1330)
#define BOOST_FUSION_DEQUE_DETAIL_KEYED_ELEMENT_26112006_1330

#include <boost/fusion/support/detail/access.hpp>
#include <boost/fusion/iterator/deref.hpp>
#include <boost/fusion/iterator/next.hpp>

namespace boost { namespace fusion
{
    struct fusion_sequence_tag;
}}

namespace boost { namespace fusion { namespace detail
{
    struct nil_keyed_element
    {
        typedef fusion_sequence_tag tag;
        void get();

        template<typename It>
        static nil_keyed_element
        from_iterator(It const&)
        {
            return nil_keyed_element();
        }
    };

    template <typename Key, typename Value, typename Rest>
    struct keyed_element : Rest
    {
        typedef Rest base;
        typedef fusion_sequence_tag tag;
        using Rest::get;

        template <typename It>
        static keyed_element
        from_iterator(It const& it)
        {
            return keyed_element(
                *it, base::from_iterator(fusion::next(it)));
        }

        keyed_element(keyed_element const& rhs)
          : Rest(rhs.get_base()), value_(rhs.value_)
        {}

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        keyed_element(keyed_element&& rhs)
          : Rest(std::forward<Rest>(rhs.forward_base()))
          , value_(std::forward<Value>(rhs.value_))
        {}
#endif

        template <typename U, typename Rst>
        keyed_element(keyed_element<Key, U, Rst> const& rhs)
          : Rest(rhs.get_base()), value_(rhs.value_)
        {}

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#endif

        Rest& get_base()
        {
            return *this;
        }

        Rest const& get_base() const
        {
            return *this;
        }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        Rest&& forward_base()
        {
            return std::forward<Rest>(*static_cast<Rest*>(this));
        }
#endif

        typename cref_result<Value>::type get(Key) const
        {
            return value_;
        }

        typename ref_result<Value>::type get(Key)
        {
            return value_;
        }

        keyed_element(
            typename detail::call_param<Value>::type value
          , Rest const& rest)
            : Rest(rest), value_(value)
        {}

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        keyed_element(Value&& value, Rest&& rest)
            : Rest(std::forward<Rest>(rest))
            , value_(std::forward<Value>(value))
        {}
#endif

        keyed_element()
            : Rest(), value_()
        {}

        template<typename U, typename Rst>
        keyed_element& operator=(keyed_element<Key, U, Rst> const& rhs)
        {
            base::operator=(static_cast<Rst const&>(rhs)); // cast for msvc-7.1
            value_ = rhs.value_;
            return *this;
        }

        keyed_element& operator=(keyed_element const& rhs)
        {
            base::operator=(rhs);
            value_ = rhs.value_;
            return *this;
        }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        keyed_element& operator=(keyed_element&& rhs)
        {
            base::operator=(std::forward<keyed_element>(rhs));
            value_ = std::forward<Value>(rhs.value_);
            return *this;
        }
#endif

        Value value_;
    };

    template<typename Elem, typename Key>
    struct keyed_element_value_at
      : keyed_element_value_at<typename Elem::base, Key>
    {};

    template<typename Key, typename Value, typename Rest>
    struct keyed_element_value_at<keyed_element<Key, Value, Rest>, Key>
    {
        typedef Value type;
    };
}}}

#endif
