//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  This example is the equivalent to the following lex program:
//
//       %{
//       /* INITIAL is the default start state.  COMMENT is our new  */
//       /* state where we remove comments.                          */
//       %}
// 
//       %s COMMENT
//       %%
//       <INITIAL>"//".*    ;
//       <INITIAL>"/*"      BEGIN COMMENT; 
//       <INITIAL>.         ECHO;
//       <INITIAL>[\n]      ECHO;
//       <COMMENT>"*/"      BEGIN INITIAL;
//       <COMMENT>.         ;
//       <COMMENT>[\n]      ;
//       %%
// 
//       main() 
//       {
//         yylex();
//       }
//
//  Its purpose is to strip comments out of C code.
//
//  Additionally this example demonstrates the use of lexer states to structure
//  the lexer definition.

// #define BOOST_SPIRIT_LEXERTL_DEBUG

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/phoenix_core.hpp>

#include <iostream>
#include <string>

#include "example.hpp"

using namespace boost::spirit;  

///////////////////////////////////////////////////////////////////////////////
//  Token definition: We use the lexertl based lexer engine as the underlying 
//                    lexer type.
///////////////////////////////////////////////////////////////////////////////
enum tokenids 
{
    IDANY = lex::min_token_id + 10,
    IDEOL = lex::min_token_id + 11
};

///////////////////////////////////////////////////////////////////////////////
// Simple custom semantic action function object used to print the matched
// input sequence for a particular token
template <typename Char, typename Traits>
struct echo_input_functor
{
    echo_input_functor (std::basic_ostream<Char, Traits>& os_)
      : os(os_) {}

    // This is called by the semantic action handling code during the lexing
    template <typename Iterator, typename Context>
    void operator()(Iterator const& b, Iterator const& e
      , BOOST_SCOPED_ENUM(boost::spirit::lex::pass_flags)&
      , std::size_t&, Context&) const
    {
        os << std::string(b, e);
    }

    std::basic_ostream<Char, Traits>& os;
};

template <typename Char, typename Traits>
inline echo_input_functor<Char, Traits> 
echo_input(std::basic_ostream<Char, Traits>& os)
{
    return echo_input_functor<Char, Traits>(os);
}

///////////////////////////////////////////////////////////////////////////////
// Another simple custom semantic action function object used to switch the 
// state of the lexer 
struct set_lexer_state
{
    set_lexer_state(char const* state_)
      : state(state_) {}

    // This is called by the semantic action handling code during the lexing
    template <typename Iterator, typename Context>
    void operator()(Iterator const&, Iterator const&
      , BOOST_SCOPED_ENUM(boost::spirit::lex::pass_flags)&
      , std::size_t&, Context& ctx) const
    {
        ctx.set_state_name(state.c_str());
    }

    std::string state;
};

///////////////////////////////////////////////////////////////////////////////
template <typename Lexer>
struct strip_comments_tokens : lex::lexer<Lexer>
{
    strip_comments_tokens()
      : strip_comments_tokens::base_type(lex::match_flags::match_default)
    {
        // define tokens and associate them with the lexer
        cppcomment = "\\/\\/[^\n]*";    // '//[^\n]*'
        ccomment = "\\/\\*";            // '/*'
        endcomment = "\\*\\/";          // '*/'
        any = std::string(".");
        eol = "\n";

        // The following tokens are associated with the default lexer state 
        // (the "INITIAL" state). Specifying 'INITIAL' as a lexer state is 
        // strictly optional.
        this->self 
            =   cppcomment
            |   ccomment    [ set_lexer_state("COMMENT") ]
            |   eol         [ echo_input(std::cout) ]
            |   any         [ echo_input(std::cout) ]
            ;

        // The following tokens are associated with the lexer state 'COMMENT'.
        this->self("COMMENT") 
            =   endcomment  [ set_lexer_state("INITIAL") ]
            |   "\n"
            |   std::string(".") 
            ;
    }

    lex::token_def<> cppcomment, ccomment, endcomment, any, eol;
};

  ///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // iterator type used to expose the underlying input stream
    typedef std::string::iterator base_iterator_type;

    // lexer type
    typedef 
        lex::lexertl::actor_lexer<lex::lexertl::token<base_iterator_type> > 
    lexer_type;

    // now we use the types defined above to create the lexer and grammar
    // object instances needed to invoke the parsing process
    strip_comments_tokens<lexer_type> strip_comments;             // Our lexer

    // No parsing is done alltogether, everything happens in the lexer semantic
    // actions.
    std::string str (read_from_file(1 == argc ? "strip_comments.input" : argv[1]));
    base_iterator_type first = str.begin();
    bool r = lex::tokenize(first, str.end(), strip_comments);

    if (!r) {
        std::string rest(first, str.end());
        std::cerr << "Lexical analysis failed\n" << "stopped at: \"" 
                  << rest << "\"\n";
    }
    return 0;
}



