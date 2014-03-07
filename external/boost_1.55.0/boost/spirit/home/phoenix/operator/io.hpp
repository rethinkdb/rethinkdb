/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_OPERATOR_IO_HPP
#define PHOENIX_OPERATOR_IO_HPP

#include <boost/spirit/home/phoenix/operator/detail/io.hpp>

namespace boost { namespace phoenix
{
///////////////////////////////////////////////////////////////////////////////
//
//  overloads for std::basic_ostream and std::basic_istream
//
///////////////////////////////////////////////////////////////////////////////
    template <typename T0, typename T1>
    inline typename detail::enable_if_ostream<T0, T1>::type
    operator<<(T0& a0, actor<T1> const& a1)
    {
        return compose<shift_left_eval>(phoenix::ref(a0), a1);
    }

    template <typename T0, typename T1>
    inline typename detail::enable_if_istream<T0, T1>::type
    operator>>(T0& a0, actor<T1> const& a1)
    {
        return compose<shift_right_eval>(phoenix::ref(a0), a1);
    }

    // resolve ambiguities with fusion.
    template <typename T1>
    inline typename detail::enable_if_ostream<std::ostream, T1>::type
    operator<<(std::ostream& a0, actor<T1> const& a1)
    {
        return compose<shift_left_eval>(phoenix::ref(a0), a1);
    }

    template <typename T1>
    inline typename detail::enable_if_istream<std::istream, T1>::type
        operator>>(std::istream& a0, actor<T1> const& a1)
    {
        return compose<shift_right_eval>(phoenix::ref(a0), a1);
    }

///////////////////////////////////////////////////////////////////////////////
//
//  overloads for I/O manipulators.
//
///////////////////////////////////////////////////////////////////////////////
    template <typename T0>
    inline actor<typename as_composite<
        shift_left_eval, actor<T0>, detail::omanip_type>::type>
    operator<<(actor<T0> const& a0, detail::omanip_type a1)
    {
        return compose<shift_left_eval>(a0, a1);
    }

    template <typename T0>
    inline actor<typename as_composite<
        shift_left_eval, actor<T0>, detail::iomanip_type>::type>
    operator<<(actor<T0> const& a0, detail::iomanip_type a1)
    {
        return compose<shift_left_eval>(a0, a1);
    }

    template <typename T0>
    inline actor<typename as_composite<
        shift_right_eval, actor<T0>, detail::imanip_type>::type>
    operator>>(actor<T0> const& a0, detail::imanip_type a1)
    {
        return compose<shift_right_eval>(a0, a1);
    }

    template <typename T0>
    inline actor<typename as_composite<
        shift_right_eval, actor<T0>, detail::iomanip_type>::type>
    operator>>(actor<T0> const& a0, detail::iomanip_type a1)
    {
        return compose<shift_right_eval>(a0, a1);
    }
}}

#endif
