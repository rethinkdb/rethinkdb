/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   support/regex.hpp
 * \author Andrey Semashev
 * \date   18.07.2009
 *
 * This header enables Boost.Regex support for Boost.Log.
 */

#ifndef BOOST_LOG_SUPPORT_REGEX_HPP_INCLUDED_
#define BOOST_LOG_SUPPORT_REGEX_HPP_INCLUDED_

#include <boost/regex.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/utility/functional/matches.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

//! The trait verifies if the type can be converted to a Boost.Regex expression
template< typename T >
struct is_regex< T, true >
{
private:
    typedef char yes_type;
    struct no_type { char dummy[2]; };

    template< typename CharT, typename TraitsT >
    static yes_type check_regex(basic_regex< CharT, TraitsT > const&);
    static no_type check_regex(...);
    static T& get_T();

public:
    enum { value = sizeof(check_regex(get_T())) == sizeof(yes_type) };
    typedef mpl::bool_< value > type;
};

//! The regex matching functor implementation
template< >
struct matches_fun_impl< boost_regex_expression_tag >
{
    template< typename StringT, typename CharT, typename TraitsT >
    static bool matches(
        StringT const& str,
        basic_regex< CharT, TraitsT > const& expr,
        match_flag_type flags = match_default)
    {
        return boost::regex_match(str.begin(), str.end(), expr, flags);
    }
};

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_SUPPORT_REGEX_HPP_INCLUDED_
