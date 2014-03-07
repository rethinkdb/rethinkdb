/*=============================================================================
    Copyright (c) 2001-2010 Hartmut Kaiser
    Copyright (c) 2001-2010 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_TEST_MATCH_MANIP_HPP)
#define BOOST_SPIRIT_TEST_MATCH_MANIP_HPP

#include <boost/config/warning_disable.hpp>

#include <boost/spirit/include/support_argument.hpp>
#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_stream.hpp>
#include <boost/spirit/include/qi_match.hpp>
#include <boost/spirit/include/qi_match_auto.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>

#include <string>
#include <sstream>
#include <vector>
#include <list>

#include <boost/detail/lightweight_test.hpp>

///////////////////////////////////////////////////////////////////////////////
template <typename Char, typename Expr>
bool test(Char const *toparse, Expr const& expr)
{
    namespace spirit = boost::spirit;
    BOOST_SPIRIT_ASSERT_MATCH(spirit::qi::domain, Expr);

    std::istringstream istrm(toparse);
    istrm.unsetf(std::ios::skipws);
    istrm >> spirit::qi::compile<spirit::qi::domain>(expr);
    return istrm.good() || istrm.eof();
}

template <typename Char, typename Expr, typename CopyExpr, typename CopyAttr
  , typename Skipper, typename Attribute>
bool test(Char const *toparse,
    boost::spirit::qi::detail::match_manip<
        Expr, CopyExpr, CopyAttr, Skipper, Attribute> const& mm)
{
    std::istringstream istrm(toparse);
    istrm.unsetf(std::ios::skipws);
    istrm >> mm;
    return istrm.good() || istrm.eof();
}

///////////////////////////////////////////////////////////////////////////////
bool is_list_ok(std::list<char> const& l)
{
    std::list<char>::const_iterator cit = l.begin();
    if (cit == l.end() || *cit != 'a')
        return false;
    if (++cit == l.end() || *cit != 'b')
        return false;

    return ++cit != l.end() && *cit == 'c';
}

#endif
