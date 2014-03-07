/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_CONJURE_EXPRESSION_HPP)
#define BOOST_SPIRIT_CONJURE_EXPRESSION_HPP

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
    template <typename Iterator, typename Lexer>
    struct expression : qi::grammar<Iterator, ast::expression()>
    {
        typedef error_handler<typename Lexer::base_iterator_type, Iterator>
            error_handler_type;

        expression(error_handler_type& error_handler, Lexer const& l);

        Lexer const& lexer;

        qi::rule<Iterator, ast::expression()> expr;
        qi::rule<Iterator, ast::operand()> unary_expr, primary_expr;
        qi::rule<Iterator, ast::function_call()> function_call;
        qi::rule<Iterator, std::list<ast::expression>()> argument_list;
        qi::rule<Iterator, std::string()> identifier;
    };
}}

#endif


