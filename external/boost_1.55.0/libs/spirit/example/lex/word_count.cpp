//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  This example is the equivalent to the following lex program:
/*
//[wcp_flex_version
    %{
        int c = 0, w = 0, l = 0;
    %}
    word   [^ \t\n]+
    eol    \n
    %%
    {word} { ++w; c += yyleng; }
    {eol}  { ++c; ++l; }
    .      { ++c; }
    %%
    main()
    {
        yylex();
        printf("%d %d %d\n", l, w, c);
    }
//]
*/
//  Its purpose is to do the word count function of the wc command in UNIX. It 
//  prints the number of lines, words and characters in a file. 
//
//  The example additionally demonstrates how to use the add_pattern(...)(...)
//  syntax to define lexer patterns. These patterns are essentially parameter-
//  less 'macros' for regular expressions, allowing to simplify their 
//  definition.

// #define BOOST_SPIRIT_LEXERTL_DEBUG
#define BOOST_VARIANT_MINIMIZE_SIZE

#include <boost/config/warning_disable.hpp>
//[wcp_includes
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/phoenix_container.hpp>
//]

#include <iostream>
#include <string>

#include "example.hpp"

//[wcp_namespaces
using namespace boost::spirit;
using namespace boost::spirit::ascii;
//]

///////////////////////////////////////////////////////////////////////////////
//  Token definition: We use the lexertl based lexer engine as the underlying 
//                    lexer type.
///////////////////////////////////////////////////////////////////////////////
//[wcp_token_ids
enum tokenids 
{
    IDANY = lex::min_token_id + 10
};
//]

//[wcp_token_definition
template <typename Lexer>
struct word_count_tokens : lex::lexer<Lexer>
{
    word_count_tokens()
    {
        // define patterns (lexer macros) to be used during token definition 
        // below
        this->self.add_pattern
            ("WORD", "[^ \t\n]+")
        ;

        // define tokens and associate them with the lexer
        word = "{WORD}";    // reference the pattern 'WORD' as defined above

        // this lexer will recognize 3 token types: words, newlines, and 
        // everything else
        this->self.add
            (word)          // no token id is needed here
            ('\n')          // characters are usable as tokens as well
            (".", IDANY)    // string literals will not be esacped by the library
        ;
    }

    // the token 'word' exposes the matched string as its parser attribute
    lex::token_def<std::string> word;
};
//]

///////////////////////////////////////////////////////////////////////////////
//  Grammar definition
///////////////////////////////////////////////////////////////////////////////
//[wcp_grammar_definition
template <typename Iterator>
struct word_count_grammar : qi::grammar<Iterator>
{
    template <typename TokenDef>
    word_count_grammar(TokenDef const& tok)
      : word_count_grammar::base_type(start)
      , c(0), w(0), l(0)
    {
        using boost::phoenix::ref;
        using boost::phoenix::size;

        start =  *(   tok.word          [++ref(w), ref(c) += size(_1)]
                  |   lit('\n')         [++ref(c), ++ref(l)] 
                  |   qi::token(IDANY)  [++ref(c)]
                  )
              ;
    }

    std::size_t c, w, l;
    qi::rule<Iterator> start;
};
//]

///////////////////////////////////////////////////////////////////////////////
//[wcp_main
int main(int argc, char* argv[])
{
/*<  Define the token type to be used: `std::string` is available as the 
     type of the token attribute 
>*/  typedef lex::lexertl::token<
        char const*, boost::mpl::vector<std::string>
    > token_type;

/*<  Define the lexer type to use implementing the state machine
>*/  typedef lex::lexertl::lexer<token_type> lexer_type;

/*<  Define the iterator type exposed by the lexer type
>*/  typedef word_count_tokens<lexer_type>::iterator_type iterator_type;

    // now we use the types defined above to create the lexer and grammar
    // object instances needed to invoke the parsing process
    word_count_tokens<lexer_type> word_count;          // Our lexer
    word_count_grammar<iterator_type> g (word_count);  // Our parser 

    // read in the file int memory
    std::string str (read_from_file(1 == argc ? "word_count.input" : argv[1]));
    char const* first = str.c_str();
    char const* last = &first[str.size()];

/*<  Parsing is done based on the the token stream, not the character 
     stream read from the input. The function `tokenize_and_parse()` wraps
     the passed iterator range `[first, last)` by the lexical analyzer and 
     uses its exposed iterators to parse the toke stream.
>*/  bool r = lex::tokenize_and_parse(first, last, word_count, g);

    if (r) {
        std::cout << "lines: " << g.l << ", words: " << g.w 
                  << ", characters: " << g.c << "\n";
    }
    else {
        std::string rest(first, last);
        std::cerr << "Parsing failed\n" << "stopped at: \"" 
                  << rest << "\"\n";
    }
    return 0;
}
//]
