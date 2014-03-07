//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  This example is the equivalent to the following flex program:
/*
//[wcf_flex_version
    %{
        #define ID_WORD 1000
        #define ID_EOL  1001
        #define ID_CHAR 1002
        int c = 0, w = 0, l = 0;
    %}
    %%
    [^ \t\n]+  { return ID_WORD; }
    \n         { return ID_EOL; }
    .          { return ID_CHAR; }
    %%
    bool count(int tok)
    {
        switch (tok) {
        case ID_WORD: ++w; c += yyleng; break;
        case ID_EOL:  ++l; ++c; break;
        case ID_CHAR: ++c; break;
        default:
            return false;
        }
        return true;
    }
    void main()
    {
        int tok = EOF;
        do {
            tok = yylex();
            if (!count(tok))
                break;
        } while (EOF != tok);
        printf("%d %d %d\n", l, w, c);
    }
//]
*/
//  Its purpose is to do the word count function of the wc command in UNIX. It 
//  prints the number of lines, words and characters in a file. 
//
//  This examples shows how to use the tokenize() function together with a 
//  simple functor, which gets executed whenever a token got matched in the 
//  input sequence. 

// #define BOOST_SPIRIT_LEXERTL_DEBUG

#include <boost/config/warning_disable.hpp>
//[wcf_includes
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
//]

#include <iostream>
#include <string>

#include "example.hpp"

//[wcf_namespaces
namespace lex = boost::spirit::lex;
//]

///////////////////////////////////////////////////////////////////////////////
//  Token id definitions
///////////////////////////////////////////////////////////////////////////////
//[wcf_token_ids
enum token_ids
{
    ID_WORD = 1000,
    ID_EOL,
    ID_CHAR
};
//]

//[wcf_token_definition
/*` The template `word_count_tokens` defines three different tokens: 
    `ID_WORD`, `ID_EOL`, and `ID_CHAR`, representing a word (anything except 
    a whitespace or a newline), a newline character, and any other character 
    (`ID_WORD`, `ID_EOL`, and `ID_CHAR` are enum values representing the token 
    ids, but could be anything else convertible to an integer as well).
    The direct base class of any token definition class needs to be the 
    template `lex::lexer<>`, where the corresponding template parameter (here:
    `lex::lexertl::lexer<BaseIterator>`) defines which underlying lexer engine has 
    to be used to provide the required state machine functionality. In this 
    example we use the Lexertl based lexer engine as the underlying lexer type. 
*/
template <typename Lexer>
struct word_count_tokens : lex::lexer<Lexer>
{
    word_count_tokens()
    {
        // define tokens (the regular expression to match and the corresponding
        // token id) and add them to the lexer 
        this->self.add
            ("[^ \t\n]+", ID_WORD) // words (anything except ' ', '\t' or '\n')
            ("\n", ID_EOL)         // newline characters
            (".", ID_CHAR)         // anything else is a plain character
        ;
    }
};
//]

//[wcf_functor
/*` In this example the struct 'counter' is used as a functor counting the 
    characters, words and lines in the analyzed input sequence by identifying 
    the matched tokens as passed from the /Spirit.Lex/ library. 
*/
struct counter
{
//<- this is an implementation detail specific to boost::bind and doesn't show 
//   up in the documentation
    typedef bool result_type;
//->
    // the function operator gets called for each of the matched tokens
    // c, l, w are references to the counters used to keep track of the numbers
    template <typename Token>
    bool operator()(Token const& t, std::size_t& c, std::size_t& w, std::size_t& l) const
    {
        switch (t.id()) {
        case ID_WORD:       // matched a word
        // since we're using a default token type in this example, every 
        // token instance contains a `iterator_range<BaseIterator>` as its token
        // attribute pointing to the matched character sequence in the input 
            ++w; c += t.value().size(); 
            break;
        case ID_EOL:        // matched a newline character
            ++l; ++c; 
            break;
        case ID_CHAR:       // matched something else
            ++c; 
            break;
        }
        return true;        // always continue to tokenize
    }
};
//]

///////////////////////////////////////////////////////////////////////////////
//[wcf_main
/*` The main function simply loads the given file into memory (as a 
    `std::string`), instantiates an instance of the token definition template
    using the correct iterator type (`word_count_tokens<char const*>`),
    and finally calls `lex::tokenize`, passing an instance of the counter function
    object. The return value of `lex::tokenize()` will be `true` if the 
    whole input sequence has been successfully tokenized, and `false` otherwise.
*/
int main(int argc, char* argv[])
{
    // these variables are used to count characters, words and lines
    std::size_t c = 0, w = 0, l = 0;

    // read input from the given file
    std::string str (read_from_file(1 == argc ? "word_count.input" : argv[1]));

    // create the token definition instance needed to invoke the lexical analyzer
    word_count_tokens<lex::lexertl::lexer<> > word_count_functor;

    // tokenize the given string, the bound functor gets invoked for each of 
    // the matched tokens
    char const* first = str.c_str();
    char const* last = &first[str.size()];
    bool r = lex::tokenize(first, last, word_count_functor, 
        boost::bind(counter(), _1, boost::ref(c), boost::ref(w), boost::ref(l)));

    // print results
    if (r) {
        std::cout << "lines: " << l << ", words: " << w 
                  << ", characters: " << c << "\n";
    }
    else {
        std::string rest(first, last);
        std::cout << "Lexical analysis failed\n" << "stopped at: \"" 
                  << rest << "\"\n";
    }
    return 0;
}
//]

