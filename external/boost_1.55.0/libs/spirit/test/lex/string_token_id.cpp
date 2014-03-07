//  Copyright (c) 2001-2010 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// #define BOOST_SPIRIT_LEXERTL_DEBUG
#define BOOST_VARIANT_MINIMIZE_SIZE

#include <boost/detail/lightweight_test.hpp>
#include <boost/config/warning_disable.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/phoenix_container.hpp>

#include <iostream>
#include <string>

namespace qi = boost::spirit::qi;
namespace lex = boost::spirit::lex;

enum tokenids
{
    IDWORD = lex::min_token_id, 
    IDCHAR,
    IDANY
};

template <typename Lexer>
struct word_count_tokens : lex::lexer<Lexer>
{
    word_count_tokens()
    {
        this->self.add_pattern
            ("TEST", "A")
        ;

        this->self = 
                lex::string("{TEST}", IDWORD) 
            |   lex::char_('a', IDCHAR)
            |   lex::string(".", IDANY)
            ;
    }
};

template <typename Iterator>
struct word_count_grammar : qi::grammar<Iterator>
{
    template <typename TokenDef>
    word_count_grammar(TokenDef const&)
      : word_count_grammar::base_type(start)
      , w(0), c(0), a(0)
    {
        using boost::phoenix::ref;
        using qi::token;

        start =  *(   token(IDWORD) [++ref(w)]
                  |   token(IDCHAR) [++ref(c)]
                  |   token(IDANY)  [++ref(a)]
                  )
              ;
    }
    std::size_t w, c, a;
    qi::rule<Iterator> start;
};


int main()
{
    typedef lex::lexertl::token<
        const char*, boost::mpl::vector<std::string>
    > token_type;

    typedef lex::lexertl::lexer<token_type> lexer_type;
    typedef word_count_tokens<lexer_type>::iterator_type iterator_type;
    word_count_tokens<lexer_type> word_count;          // Our lexer
    word_count_grammar<iterator_type> g (word_count);  // Our parser

    std::string str ("AaBCD");
    char const* first = str.c_str();
    char const* last = &first[str.size()];

    BOOST_TEST(lex::tokenize_and_parse(first, last, word_count, g));
    BOOST_TEST(g.w == 1 && g.c == 1 && g.a == 3);

    return boost::report_errors();
}
