/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_TEST_PARSER_SEP_24_2007_0558PM)
#define BOOST_SPIRIT_TEST_PARSER_SEP_24_2007_0558PM

#include <boost/spirit/include/qi_parse.hpp>
#include <boost/spirit/include/qi_what.hpp>

namespace spirit_test
{
    template <typename Char, typename Parser, typename Lexer>
    bool test_parser(Char const* in, Parser const& p, Lexer& lex, 
        bool full_match = true)
    {
        // we don't care about the result of the "what" function.
        // we only care that all parsers have it:
        boost::spirit::qi::what(p);

        std::string str (in);
        std::string::iterator it_in = str.begin();
        std::string::iterator end_in = str.end();

        typedef typename Lexer::iterator_type iterator_type;

        iterator_type iter = lex.begin(it_in, end_in);
        iterator_type end = lex.end();

        return boost::spirit::qi::parse(iter, end, p)
            && (!full_match || (iter == end));
    }

    template <typename Char, typename Parser, typename Lexer, typename Skipper>
    bool test_parser(Char const* in, Parser const& p, Lexer& lex,
          Skipper const& s, bool full_match = true)
    {
        // we don't care about the result of the "what" function.
        // we only care that all parsers have it:
        boost::spirit::qi::what(p);

        std::string str (in);
        std::string::iterator it_in = str.begin();
        std::string::iterator end_in = str.end();

        typedef typename Lexer::iterator_type iterator_type;

        iterator_type iter = lex.begin(it_in, end_in);
        iterator_type end = lex.end();

        return boost::spirit::qi::phrase_parse(iter, end, p, s)
            && (!full_match || (iter == end));
    }

}

#endif
