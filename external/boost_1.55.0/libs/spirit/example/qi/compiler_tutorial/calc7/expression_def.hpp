/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include "expression.hpp"
#include "error_handler.hpp"
#include "annotation.hpp"
#include <boost/spirit/include/phoenix_function.hpp>

namespace client { namespace parser
{
    template <typename Iterator>
    expression<Iterator>::expression(error_handler<Iterator>& error_handler)
      : expression::base_type(expr)
    {
        qi::_1_type _1;
        qi::_2_type _2;
        qi::_3_type _3;
        qi::_4_type _4;

        qi::char_type char_;
        qi::uint_type uint_;
        qi::_val_type _val;
        qi::raw_type raw;
        qi::lexeme_type lexeme;
        qi::alpha_type alpha;
        qi::alnum_type alnum;

        using qi::on_error;
        using qi::on_success;
        using qi::fail;
        using boost::phoenix::function;

        typedef function<client::error_handler<Iterator> > error_handler_function;
        typedef function<client::annotation<Iterator> > annotation_function;

        expr =
            additive_expr.alias()
            ;

        additive_expr =
            multiplicative_expr
            >> *(   (char_('+') > multiplicative_expr)
                |   (char_('-') > multiplicative_expr)
                )
            ;

        multiplicative_expr =
            unary_expr
            >> *(   (char_('*') > unary_expr)
                |   (char_('/') > unary_expr)
                )
            ;

        unary_expr =
                primary_expr
            |   (char_('-') > primary_expr)
            |   (char_('+') > primary_expr)
            ;

        primary_expr =
                uint_
            |   identifier
            |   '(' > expr > ')'
            ;

        identifier =
            raw[lexeme[(alpha | '_') >> *(alnum | '_')]]
            ;

        // Debugging and error handling and reporting support.
        BOOST_SPIRIT_DEBUG_NODES(
            (expr)
            (additive_expr)
            (multiplicative_expr)
            (unary_expr)
            (primary_expr)
            (identifier)
        );

        // Error handling: on error in expr, call error_handler.
        on_error<fail>(expr,
            error_handler_function(error_handler)(
                "Error! Expecting ", _4, _3));

        // Annotation: on success in primary_expr, call annotation.
        on_success(primary_expr,
            annotation_function(error_handler.iters)(_val, _1));
    }
}}


