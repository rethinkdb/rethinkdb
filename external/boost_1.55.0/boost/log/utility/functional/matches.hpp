/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   matches.hpp
 * \author Andrey Semashev
 * \date   30.03.2008
 *
 * This header contains a predicate for checking if the provided string matches a regular expression.
 */

#ifndef BOOST_LOG_UTILITY_FUNCTIONAL_MATCHES_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_FUNCTIONAL_MATCHES_HPP_INCLUDED_

#include <boost/mpl/bool.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

//! This tag type is used if an expression is not supported for matching against strings
struct unsupported_match_expression_tag;
//! This tag type is used if an expression is recognized as a Boost.Regex expression
struct boost_regex_expression_tag;
//! This tag type is used if an expression is recognized as a Boost.Xpressive expression
struct boost_xpressive_expression_tag;
//! This tag type is used if an expression is recognized as a Boost.Spirit (classic) expression
struct boost_spirit_classic_expression_tag;
//! This tag type is used if an expression is recognized as a Boost.Spirit.Qi expression
struct boost_spirit_qi_expression_tag;

//! Preliminary declaration of a trait that detects if an expression is a Boost.Regex expression
template< typename, bool = true >
struct is_regex :
    public mpl::false_
{
};
//! Preliminary declaration of a trait that detects if an expression is a Boost.Xpressive expression
template< typename, bool = true >
struct is_xpressive_regex :
    public mpl::false_
{
};
//! Preliminary declaration of a trait that detects if an expression is a Boost.Spirit (classic) expression
template< typename, bool = true >
struct is_spirit_classic_parser :
    public mpl::false_
{
};
//! Preliminary declaration of a trait that detects if an expression is a Boost.Spirit.Qi expression
template< typename, bool = true >
struct is_spirit_qi_parser :
    public mpl::false_
{
};

//! The regex matching functor implementation
template< typename TagT >
struct matches_fun_impl;

} // namespace aux

//! The regex matching functor
struct matches_fun
{
    typedef bool result_type;

private:
    //! A traits to obtain the tag of the expression
    template< typename ExpressionT >
    struct match_traits
    {
        typedef typename mpl::eval_if<
            aux::is_regex< ExpressionT >,
            mpl::identity< aux::boost_regex_expression_tag >,
            mpl::eval_if<
                aux::is_xpressive_regex< ExpressionT >,
                mpl::identity< aux::boost_xpressive_expression_tag >,
                mpl::eval_if<
                    aux::is_spirit_classic_parser< ExpressionT >,
                    mpl::identity< aux::boost_spirit_classic_expression_tag >,
                    mpl::if_<
                        aux::is_spirit_qi_parser< ExpressionT >,
                        aux::boost_spirit_qi_expression_tag,
                        aux::unsupported_match_expression_tag
                    >
                >
            >
        >::type tag_type;
    };

public:
    template< typename StringT, typename ExpressionT >
    bool operator() (StringT const& str, ExpressionT const& expr) const
    {
        typedef typename match_traits< ExpressionT >::tag_type tag_type;
        typedef aux::matches_fun_impl< tag_type > impl;
        return impl::matches(str, expr);
    }
    template< typename StringT, typename ExpressionT, typename ArgT >
    bool operator() (StringT const& str, ExpressionT const& expr, ArgT const& arg) const
    {
        typedef typename match_traits< ExpressionT >::tag_type tag_type;
        typedef aux::matches_fun_impl< tag_type > impl;
        return impl::matches(str, expr, arg);
    }
};

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_UTILITY_FUNCTIONAL_MATCHES_HPP_INCLUDED_
