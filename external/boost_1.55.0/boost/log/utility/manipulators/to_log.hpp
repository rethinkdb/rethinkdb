/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   to_log.hpp
 * \author Andrey Semashev
 * \date   06.11.2012
 *
 * This header contains the \c to_log output manipulator.
 */

#ifndef BOOST_LOG_UTILITY_MANIPULATORS_TO_LOG_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_MANIPULATORS_TO_LOG_HPP_INCLUDED_

#include <iosfwd>
#include <boost/mpl/bool.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/utility/formatting_ostream_fwd.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

/*!
 * \brief Generic manipulator for customizing output to log
 */
template< typename T, typename TagT = void >
class to_log_manip
{
public:
    //! Output value type
    typedef T value_type;
    //! Value tag type
    typedef TagT tag_type;

private:
    //! Reference to the value
    value_type const& m_value;

public:
    explicit to_log_manip(value_type const& value) : m_value(value) {}
    to_log_manip(to_log_manip const& that) : m_value(that.m_value) {}

    value_type const& get() const { return m_value; }
};

template< typename CharT, typename TraitsT, typename T, typename TagT >
inline std::basic_ostream< CharT, TraitsT >& operator<< (std::basic_ostream< CharT, TraitsT >& strm, to_log_manip< T, TagT > manip)
{
    strm << manip.get();
    return strm;
}

template< typename CharT, typename TraitsT, typename AllocatorT, typename T, typename TagT >
inline basic_formatting_ostream< CharT, TraitsT, AllocatorT >& operator<< (basic_formatting_ostream< CharT, TraitsT, AllocatorT >& strm, to_log_manip< T, TagT > manip)
{
    strm << manip.get();
    return strm;
}

template< typename T >
inline to_log_manip< T > to_log(T const& value)
{
    return to_log_manip< T >(value);
}

template< typename TagT, typename T >
inline to_log_manip< T, TagT > to_log(T const& value)
{
    return to_log_manip< T, TagT >(value);
}

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_UTILITY_MANIPULATORS_TO_LOG_HPP_INCLUDED_
