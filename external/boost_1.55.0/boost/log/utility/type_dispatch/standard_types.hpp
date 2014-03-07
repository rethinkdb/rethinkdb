/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   standard_types.hpp
 * \author Andrey Semashev
 * \date   19.05.2007
 *
 * The header contains definition of standard types supported by the library by default.
 */

#ifndef BOOST_LOG_STANDARD_TYPES_HPP_INCLUDED_
#define BOOST_LOG_STANDARD_TYPES_HPP_INCLUDED_

#include <string>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/copy.hpp>
#include <boost/mpl/back_inserter.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/utility/string_literal_fwd.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

/*!
 * An MPL-sequence of integral types of attributes, supported by default
 */
typedef mpl::vector<
    bool,
    char,
#if !defined(BOOST_NO_INTRINSIC_WCHAR_T)
    wchar_t,
#endif
    signed char,
    unsigned char,
    short,
    unsigned short,
    int,
    unsigned int,
    long,
    unsigned long
#if defined(BOOST_HAS_LONG_LONG)
    , long long
    , unsigned long long
#endif // defined(BOOST_HAS_LONG_LONG)
> integral_types;

/*!
 * An MPL-sequence of FP types of attributes, supported by default
 */
typedef mpl::vector<
    float,
    double,
    long double
> floating_point_types;

/*!
 * An MPL-sequence of all numeric types of attributes, supported by default
 */
typedef mpl::copy<
    floating_point_types,
    mpl::back_inserter< integral_types >
>::type numeric_types;

/*!
 * An MPL-sequence of string types of attributes, supported by default
 */
typedef mpl::vector<
#ifdef BOOST_LOG_USE_CHAR
    std::string,
    string_literal
#ifdef BOOST_LOG_USE_WCHAR_T
    ,
#endif
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
    std::wstring,
    wstring_literal
#endif
> string_types;

/*!
 * An MPL-sequence of all attribute value types that are supported by the library by default.
 */
typedef mpl::copy<
    string_types,
    mpl::back_inserter< numeric_types >
>::type default_attribute_types;

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_STANDARD_TYPES_HPP_INCLUDED_
