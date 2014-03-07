//  boost lockfree: copy_payload helper
//
//  Copyright (C) 2011 Tim Blechmann
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_LOCKFREE_DETAIL_COPY_PAYLOAD_HPP_INCLUDED
#define BOOST_LOCKFREE_DETAIL_COPY_PAYLOAD_HPP_INCLUDED

#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_convertible.hpp>

namespace boost    {
namespace lockfree {
namespace detail   {

struct copy_convertible
{
    template <typename T, typename U>
    static void copy(T & t, U & u)
    {
        u = t;
    }
};

struct copy_constructible_and_copyable
{
    template <typename T, typename U>
    static void copy(T & t, U & u)
    {
        u = U(t);
    }
};

template <typename T, typename U>
void copy_payload(T & t, U & u)
{
    typedef typename boost::mpl::if_<typename boost::is_convertible<T, U>::type,
                                     copy_convertible,
                                     copy_constructible_and_copyable
                                    >::type copy_type;
    copy_type::copy(t, u);
}

template <typename T>
struct consume_via_copy
{
    consume_via_copy(T & out):
        out(out)
    {}

    template <typename U>
    void operator()(U & element)
    {
        copy_payload(element, out);
    }

    T &  out;
};


}}}

#endif  /* BOOST_LOCKFREE_DETAIL_COPY_PAYLOAD_HPP_INCLUDED */
