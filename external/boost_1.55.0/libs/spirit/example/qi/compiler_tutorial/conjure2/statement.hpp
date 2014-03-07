/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_CONJURE_STATEMENT_HPP)
#define BOOST_SPIRIT_CONJURE_STATEMENT_HPP

#include "expression.hpp"

namespace client { namespace parser
{
    ///////////////////////////////////////////////////////////////////////////////
    //  The statement grammar
    ///////////////////////////////////////////////////////////////////////////////
    template <typename Iterator, typename Lexer>
    struct statement : qi::grammar<Iterator, ast::statement_list()>
    {
        typedef error_handler<typename Lexer::base_iterator_type, Iterator>
            error_handler_type;

        statement(error_handler_type& error_handler, Lexer const& l);

        expression<Iterator, Lexer> expr;

        qi::rule<Iterator, ast::statement_list()>
            statement_list, compound_statement;

        qi::rule<Iterator, ast::statement()> statement_;
        qi::rule<Iterator, ast::variable_declaration()> variable_declaration;
        qi::rule<Iterator, ast::assignment()> assignment;
        qi::rule<Iterator, ast::if_statement()> if_statement;
        qi::rule<Iterator, ast::while_statement()> while_statement;
        qi::rule<Iterator, ast::return_statement()> return_statement;
    };
}}

#endif


