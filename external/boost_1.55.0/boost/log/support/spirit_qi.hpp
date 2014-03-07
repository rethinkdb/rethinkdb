/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   support/spirit_qi.hpp
 * \author Andrey Semashev
 * \date   19.07.2009
 *
 * This header enables Boost.Spirit.Qi support for Boost.Log.
 */

#ifndef BOOST_LOG_SUPPORT_SPIRIT_QI_HPP_INCLUDED_
#define BOOST_LOG_SUPPORT_SPIRIT_QI_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>
#include <boost/log/utility/functional/matches.hpp>
#include <boost/spirit/include/qi_parse.hpp>
#include <boost/spirit/include/qi_domain.hpp>
#include <boost/spirit/include/support_component.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

//! The trait verifies if the type can be converted to a Boost.Spirit.Qi parser
template< typename T >
struct is_spirit_qi_parser< T, true > :
    public spirit::traits::is_component< spirit::qi::domain, T >
{
};

//! The matching functor implementation
template< >
struct matches_fun_impl< boost_spirit_qi_expression_tag >
{
    template< typename StringT, typename ParserT >
    static bool matches(
        StringT const& str,
        ParserT const& expr)
    {
        typedef typename StringT::const_iterator const_iterator;
        const_iterator it = str.begin(), end = str.end();
        return (spirit::qi::parse(it, end, expr) && it == end);
    }
};

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_SUPPORT_SPIRIT_QI_HPP_INCLUDED_
