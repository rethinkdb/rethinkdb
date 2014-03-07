/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_OPERATOR_DETAIL_IO_HPP
#define PHOENIX_OPERATOR_DETAIL_IO_HPP

#include <boost/spirit/home/phoenix/operator/bitwise.hpp>
#include <boost/spirit/home/phoenix/core/reference.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/utility/enable_if.hpp>
#include <iostream>

namespace boost { namespace phoenix { namespace detail
{
    typedef char(&no)[1];
    typedef char(&yes)[2];

    template <typename CharType, typename CharTrait>
    yes ostream_test(std::basic_ostream<CharType, CharTrait>*);
    no ostream_test(...);

    template <typename CharType, typename CharTrait>
    yes istream_test(std::basic_istream<CharType, CharTrait>*);
    no istream_test(...);

    template <typename T>
    struct is_ostream
    {
        static T x;
        BOOST_STATIC_CONSTANT(bool,
            value = sizeof(detail::ostream_test(boost::addressof(x))) == sizeof(yes));
    };

    template <typename T>
    struct is_istream
    {
        static T x;
        BOOST_STATIC_CONSTANT(bool,
            value = sizeof(detail::istream_test(boost::addressof(x))) == sizeof(yes));
    };

    template <typename T0, typename T1>
    struct enable_if_ostream :
        enable_if<
            detail::is_ostream<T0>
          , actor<
                typename as_composite<
                    shift_left_eval
                  , actor<reference<T0> >
                  , actor<T1>
                >::type
            >
        >
    {};

    template <typename T0, typename T1>
    struct enable_if_istream :
        enable_if<
            detail::is_istream<T0>
          , actor<
                typename as_composite<
                    shift_right_eval
                  , actor<reference<T0> >
                  , actor<T1>
                >::type
            >
        >
    {};

    typedef std::ios_base&  (*iomanip_type)(std::ios_base&);
    typedef std::istream&   (*imanip_type)(std::istream&);
    typedef std::ostream&   (*omanip_type)(std::ostream&);
}}}

#endif
