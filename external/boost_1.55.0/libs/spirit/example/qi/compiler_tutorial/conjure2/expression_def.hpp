/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include "expression.hpp"
#include "error_handler.hpp"
#include "annotation.hpp"
#include <boost/spirit/include/phoenix_function.hpp>
#include <boost/spirit/include/lex_plain_token.hpp>

namespace client { namespace parser
{
    template <typename Iterator, typename Lexer>
    expression<Iterator, Lexer>::expression(
            error_handler<typename Lexer::base_iterator_type, Iterator>& error_handler
          , Lexer const& l)
      : expression::base_type(expr), lexer(l)
    {
        qi::_1_type _1;
        qi::_2_type _2;
        qi::_3_type _3;
        qi::_4_type _4;

        qi::_val_type _val;
        qi::tokenid_mask_type tokenid_mask;

        using qi::on_error;
        using qi::on_success;
        using qi::fail;
        using boost::phoenix::function;

        typedef client::error_handler<typename Lexer::base_iterator_type, Iterator>
            error_handler_type;
        typedef function<error_handler_type> error_handler_function;
        typedef function<client::annotation<Iterator> > annotation_function;

        ///////////////////////////////////////////////////////////////////////
        // Main expression grammar
        expr =
                unary_expr
            >>  *(tokenid_mask(token_ids::op_binary) > unary_expr)
            ;

        unary_expr =
                primary_expr
            |   (tokenid_mask(token_ids::op_unary) > unary_expr)
            ;

        primary_expr =
                lexer.lit_uint
            |   function_call
            |   identifier
            |   lexer.true_or_false
            |   '(' > expr > ')'
            ;

        function_call =
                (identifier >> '(')
            >   argument_list
            >   ')'
            ;

        argument_list = -(expr % ',');

        identifier = lexer.identifier;

        ///////////////////////////////////////////////////////////////////////
        // Debugging and error handling and reporting support.
        BOOST_SPIRIT_DEBUG_NODES(
            (expr)
            (unary_expr)
            (primary_expr)
            (function_call)
            (argument_list)
            (identifier)
        );

        ///////////////////////////////////////////////////////////////////////
        // Error handling: on error in expr, call error_handler.
        on_error<fail>(expr,
            error_handler_function(error_handler)(
                "Error! Expecting ", _4, _3));

        ///////////////////////////////////////////////////////////////////////
        // Annotation: on success in primary_expr, call annotation.
        on_success(primary_expr,
            annotation_function(error_handler.iters)(_val, _1));
    }
}}


