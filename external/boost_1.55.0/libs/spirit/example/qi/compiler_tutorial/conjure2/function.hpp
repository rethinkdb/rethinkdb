/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_CONJURE_FUNCTION_HPP)
#define BOOST_SPIRIT_CONJURE_FUNCTION_HPP

#include "statement.hpp"

namespace client { namespace parser
{
    ///////////////////////////////////////////////////////////////////////////////
    //  The function grammar
    ///////////////////////////////////////////////////////////////////////////////
    template <typename Iterator, typename Lexer>
    struct function : qi::grammar<Iterator, ast::function()>
    {
        typedef error_handler<typename Lexer::base_iterator_type, Iterator>
            error_handler_type;

        function(error_handler_type& error_handler, Lexer const& l);

        statement<Iterator, Lexer> body;

        qi::rule<Iterator, ast::identifier()> identifier;
        qi::rule<Iterator, std::list<ast::identifier>()> argument_list;
        qi::rule<Iterator, ast::function()> start;
    };
}}

#endif


