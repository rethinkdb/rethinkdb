//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  This example is the equivalent to the following lex program:
/*
//[wcl_flex_version
    %{
        int c = 0, w = 0, l = 0;
    %}
    %%
    [^ \t\n]+  { ++w; c += yyleng; }
    \n         { ++c; ++l; }
    .          { ++c; }
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
//  This examples shows how to use semantic actions associated with token 
//  definitions to directly attach actions to tokens. These get executed 
//  whenever the corresponding token got matched in the input sequence. Note,
//  how this example implements all functionality directly in the lexer 
//  definition without any need for a parser.

// #define BOOST_SPIRIT_LEXERTL_DEBUG

#include <boost/config/warning_disable.hpp>
//[wcl_includes
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/phoenix_algorithm.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
//]

#include <iostream>
#include <string>

#include "example.hpp"

//[wcl_namespaces
namespace lex = boost::spirit::lex;
//]

///////////////////////////////////////////////////////////////////////////////
//  Token definition: We use the lexertl based lexer engine as the underlying 
//                    lexer type.
//
//  Note, the token definition type is derived from the 'lexertl_actor_lexer'
//  template, which is a necessary to being able to use lexer semantic actions.
///////////////////////////////////////////////////////////////////////////////
struct distance_func
{
    template <typename Iterator1, typename Iterator2>
    struct result : boost::iterator_difference<Iterator1> {};

    template <typename Iterator1, typename Iterator2>
    typename result<Iterator1, Iterator2>::type 
    operator()(Iterator1& begin, Iterator2& end) const
    {
        return std::distance(begin, end);
    }
};
boost::phoenix::function<distance_func> const distance = distance_func();

//[wcl_token_definition
template <typename Lexer>
struct word_count_tokens : lex::lexer<Lexer>
{
    word_count_tokens()
      : c(0), w(0), l(0)
      , word("[^ \t\n]+")     // define tokens
      , eol("\n")
      , any(".")
    {
        using boost::spirit::lex::_start;
        using boost::spirit::lex::_end;
        using boost::phoenix::ref;

        // associate tokens with the lexer
        this->self 
            =   word  [++ref(w), ref(c) += distance(_start, _end)]
            |   eol   [++ref(c), ++ref(l)] 
            |   any   [++ref(c)]
            ;
    }

    std::size_t c, w, l;
    lex::token_def<> word, eol, any;
};
//]

///////////////////////////////////////////////////////////////////////////////
//[wcl_main
int main(int argc, char* argv[])
{

/*<  Specifying `omit` as the token attribute type generates a token class 
     not holding any token attribute at all (not even the iterator range of the 
     matched input sequence), therefore optimizing the token, the lexer, and 
     possibly the parser implementation as much as possible. Specifying 
     `mpl::false_` as the 3rd template parameter generates a token
     type and an iterator, both holding no lexer state, allowing for even more 
     aggressive optimizations. As a result the token instances contain the token 
     ids as the only data member.
>*/  typedef 
        lex::lexertl::token<char const*, lex::omit, boost::mpl::false_> 
     token_type;

/*<  This defines the lexer type to use
>*/  typedef lex::lexertl::actor_lexer<token_type> lexer_type;

/*<  Create the lexer object instance needed to invoke the lexical analysis 
>*/  word_count_tokens<lexer_type> word_count_lexer;

/*<  Read input from the given file, tokenize all the input, while discarding
     all generated tokens
>*/  std::string str (read_from_file(1 == argc ? "word_count.input" : argv[1]));
    char const* first = str.c_str();
    char const* last = &first[str.size()];

/*<  Create a pair of iterators returning the sequence of generated tokens
>*/  lexer_type::iterator_type iter = word_count_lexer.begin(first, last);
    lexer_type::iterator_type end = word_count_lexer.end();

/*<  Here we simply iterate over all tokens, making sure to break the loop
     if an invalid token gets returned from the lexer
>*/  while (iter != end && token_is_valid(*iter))
        ++iter;

    if (iter == end) {
        std::cout << "lines: " << word_count_lexer.l 
                  << ", words: " << word_count_lexer.w 
                  << ", characters: " << word_count_lexer.c 
                  << "\n";
    }
    else {
        std::string rest(first, last);
        std::cout << "Lexical analysis failed\n" << "stopped at: \"" 
                  << rest << "\"\n";
    }
    return 0;
}
//]
