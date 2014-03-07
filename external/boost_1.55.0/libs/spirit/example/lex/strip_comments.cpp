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
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_container.hpp>

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
    IDANY = lex::min_token_id + 10
};

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

        // The following tokens are associated with the default lexer state 
        // (the "INITIAL" state). Specifying 'INITIAL' as a lexer state is 
        // strictly optional.
        this->self.add
            (cppcomment)    // no explicit token id is associated
            (ccomment)
            (".", IDANY)    // IDANY is the token id associated with this token 
                            // definition
        ;

        // The following tokens are associated with the lexer state "COMMENT".
        // We switch lexer states from inside the parsing process using the 
        // in_state("COMMENT")[] parser component as shown below.
        this->self("COMMENT").add
            (endcomment)
            (".", IDANY)
        ;
    }

    lex::token_def<> cppcomment, ccomment, endcomment;
};

///////////////////////////////////////////////////////////////////////////////
//  Grammar definition
///////////////////////////////////////////////////////////////////////////////
template <typename Iterator>
struct strip_comments_grammar : qi::grammar<Iterator>
{
    template <typename TokenDef>
    strip_comments_grammar(TokenDef const& tok)
      : strip_comments_grammar::base_type(start)
    {
        // The in_state("COMMENT")[...] parser component switches the lexer 
        // state to be 'COMMENT' during the matching of the embedded parser.
        start =  *(   tok.ccomment 
                      >>  qi::in_state("COMMENT") 
                          [
                              // the lexer is in the 'COMMENT' state during
                              // matching of the following parser components
                              *token(IDANY) >> tok.endcomment 
                          ]
                  |   tok.cppcomment
                  |   qi::token(IDANY)   [ std::cout << _1 ]
                  )
              ;
    }

    qi::rule<Iterator> start;
};

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // iterator type used to expose the underlying input stream
    typedef std::string::iterator base_iterator_type;

    // lexer type
    typedef 
        lex::lexertl::lexer<lex::lexertl::token<base_iterator_type> > 
    lexer_type;

    // iterator type exposed by the lexer 
    typedef strip_comments_tokens<lexer_type>::iterator_type iterator_type;

    // now we use the types defined above to create the lexer and grammar
    // object instances needed to invoke the parsing process
    strip_comments_tokens<lexer_type> strip_comments;           // Our lexer
    strip_comments_grammar<iterator_type> g (strip_comments);   // Our parser 

    // Parsing is done based on the the token stream, not the character 
    // stream read from the input.
    std::string str (read_from_file(1 == argc ? "strip_comments.input" : argv[1]));
    base_iterator_type first = str.begin();

    bool r = lex::tokenize_and_parse(first, str.end(), strip_comments, g);

    if (r) {
        std::cout << "-------------------------\n";
        std::cout << "Parsing succeeded\n";
        std::cout << "-------------------------\n";
    }
    else {
        std::string rest(first, str.end());
        std::cout << "-------------------------\n";
        std::cout << "Parsing failed\n";
        std::cout << "stopped at: \"" << rest << "\"\n";
        std::cout << "-------------------------\n";
    }

    std::cout << "Bye... :-) \n\n";
    return 0;
}



