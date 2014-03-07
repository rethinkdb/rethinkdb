/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include "function.hpp"
#include "error_handler.hpp"
#include "annotation.hpp"

namespace client { namespace parser
{
    template <typename Iterator, typename Lexer>
    function<Iterator, Lexer>::function(
            error_handler<typename Lexer::base_iterator_type, Iterator>& error_handler
          , Lexer const& l)
      : function::base_type(start), body(error_handler, l)
    {
        qi::_1_type _1;
        qi::_2_type _2;
        qi::_3_type _3;
        qi::_4_type _4;

        qi::_val_type _val;

        using qi::on_error;
        using qi::on_success;
        using qi::fail;
        using boost::phoenix::function;

        typedef client::error_handler<typename Lexer::base_iterator_type, Iterator>
            error_handler_type;
        typedef function<error_handler_type> error_handler_function;
        typedef function<client::annotation<Iterator> > annotation_function;

        identifier = body.expr.identifier;
        argument_list = -(identifier % ',');

        start = (l.token("void") | l.token("int"))
            >   identifier
            >   '(' > argument_list > ')'
            >   '{' > body > '}'
            ;

        // Debugging and error handling and reporting support.
        BOOST_SPIRIT_DEBUG_NODES(
            (identifier)
            (argument_list)
            (start)
        );

        // Error handling: on error in start, call error_handler.
        on_error<fail>(start,
            error_handler_function(error_handler)(
                "Error! Expecting ", _4, _3));

        // Annotation: on success in start, call annotation.
        on_success(identifier,
            annotation_function(error_handler.iters)(_val, _1));
    }
}}


