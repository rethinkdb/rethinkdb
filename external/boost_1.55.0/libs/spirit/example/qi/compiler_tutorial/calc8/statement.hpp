/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_CALC8_STATEMENT_HPP)
#define BOOST_SPIRIT_CALC8_STATEMENT_HPP

#include "expression.hpp"

namespace client { namespace parser
{
    ///////////////////////////////////////////////////////////////////////////////
    //  The statement grammar
    ///////////////////////////////////////////////////////////////////////////////
    template <typename Iterator>
    struct statement : qi::grammar<Iterator, ast::statement_list(), ascii::space_type>
    {
        statement(error_handler<Iterator>& error_handler);

        expression<Iterator> expr;
        qi::rule<Iterator, ast::statement_list(), ascii::space_type>
            statement_list, compound_statement;

        qi::rule<Iterator, ast::statement(), ascii::space_type> statement_;
        qi::rule<Iterator, ast::variable_declaration(), ascii::space_type> variable_declaration;
        qi::rule<Iterator, ast::assignment(), ascii::space_type> assignment;
        qi::rule<Iterator, ast::if_statement(), ascii::space_type> if_statement;
        qi::rule<Iterator, ast::while_statement(), ascii::space_type> while_statement;
        qi::rule<Iterator, std::string(), ascii::space_type> identifier;
    };
}}

#endif


