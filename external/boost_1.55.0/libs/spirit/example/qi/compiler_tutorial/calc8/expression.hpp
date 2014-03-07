/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_CALC8_EXPRESSION_HPP)
#define BOOST_SPIRIT_CALC8_EXPRESSION_HPP

///////////////////////////////////////////////////////////////////////////////
// Spirit v2.5 allows you to suppress automatic generation
// of predefined terminals to speed up complation. With
// BOOST_SPIRIT_NO_PREDEFINED_TERMINALS defined, you are
// responsible in creating instances of the terminals that
// you need (e.g. see qi::uint_type uint_ below).
#define BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Uncomment this if you want to enable debugging
// #define BOOST_SPIRIT_QI_DEBUG
///////////////////////////////////////////////////////////////////////////////

#include <boost/spirit/include/qi.hpp>
#include "ast.hpp"
#include "error_handler.hpp"
#include <vector>

namespace client { namespace parser
{
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;

    ///////////////////////////////////////////////////////////////////////////////
    //  The expression grammar
    ///////////////////////////////////////////////////////////////////////////////
    template <typename Iterator>
    struct expression : qi::grammar<Iterator, ast::expression(), ascii::space_type>
    {
        expression(error_handler<Iterator>& error_handler);

        qi::rule<Iterator, ast::expression(), ascii::space_type>
            expr, equality_expr, relational_expr,
            logical_expr, additive_expr, multiplicative_expr
            ;

        qi::rule<Iterator, ast::operand(), ascii::space_type>
            unary_expr, primary_expr
            ;

        qi::rule<Iterator, std::string(), ascii::space_type>
            identifier
            ;

        qi::symbols<char, ast::optoken>
            equality_op, relational_op, logical_op,
            additive_op, multiplicative_op, unary_op
            ;

        qi::symbols<char>
            keywords
            ;
    };
}}

#endif


