/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

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
    template <typename Iterator>
    struct function : qi::grammar<Iterator, ast::function(), skipper<Iterator> >
    {
        function(error_handler<Iterator>& error_handler);

        statement<Iterator> body;
        qi::rule<Iterator, std::string(), skipper<Iterator> > name;
        qi::rule<Iterator, ast::identifier(), skipper<Iterator> > identifier;
        qi::rule<Iterator, std::list<ast::identifier>(), skipper<Iterator> > argument_list;
        qi::rule<Iterator, ast::function(), skipper<Iterator> > start;
    };
}}

#endif


