//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  This example shows how to create a simple lexer recognizing a couple of 
//  different tokens aimed at a simple language and how to use this lexer with 
//  a grammar. It shows how to associate attributes to tokens and how to access the 
//  token attributes from inside the grammar.
//
//  Additionally, this example demonstrates, how to define a token set usable 
//  as the skip parser during parsing, allowing to define several tokens to be 
//  ignored.
//
//  The main purpose of this example is to show how inheritance can be used to 
//  overload parts of a base grammar and add token definitions to a base lexer.
//
//  Further, it shows how you can use the 'omit' attribute type specifier 
//  for token definitions to force the token to have no attribute (expose an 
//  unused attribute).
//
//  This example recognizes a very simple programming language having 
//  assignment statements and if and while control structures. Look at the file
//  example5.input for an example.

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#include <iostream>
#include <fstream>
#include <string>

#include "example.hpp"

using namespace boost::spirit;
using boost::phoenix::val;

///////////////////////////////////////////////////////////////////////////////
//  Token definition base, defines all tokens for the base grammar below
///////////////////////////////////////////////////////////////////////////////
template <typename Lexer>
struct example5_base_tokens : lex::lexer<Lexer>
{
protected:
    // this lexer is supposed to be used as a base type only
    example5_base_tokens() {}

public:
    void init_token_definitions()
    {
        // define the tokens to match
        identifier = "[a-zA-Z_][a-zA-Z0-9_]*";
        constant = "[0-9]+";
        if_ = "if";
        while_ = "while";

        // associate the tokens and the token set with the lexer
        this->self += lex::token_def<>('(') | ')' | '{' | '}' | '=' | ';' | constant;
        this->self += if_ | while_ | identifier;

        // define the whitespace to ignore (spaces, tabs, newlines and C-style 
        // comments)
        this->self("WS")
            =   lex::token_def<>("[ \\t\\n]+") 
            |   "\\/\\*[^*]*\\*+([^/*][^*]*\\*+)*\\/"
            ;
    }

    // these tokens have no attribute
    lex::token_def<lex::omit> if_, while_;

    // The following two tokens have an associated attribute type, 'identifier'
    // carries a string (the identifier name) and 'constant' carries the 
    // matched integer value.
    //
    // Note: any token attribute type explicitly specified in a token_def<>
    //       declaration needs to be listed during token type definition as 
    //       well (see the typedef for the token_type below).
    //
    // The conversion of the matched input to an instance of this type occurs
    // once (on first access), which makes token attributes as efficient as 
    // possible. Moreover, token instances are constructed once by the lexer
    // library. From this point on tokens are passed by reference only, 
    // avoiding them being copied around.
    lex::token_def<std::string> identifier;
    lex::token_def<unsigned int> constant;
};

///////////////////////////////////////////////////////////////////////////////
//  Grammar definition base, defines a basic language
///////////////////////////////////////////////////////////////////////////////
template <typename Iterator, typename Lexer>
struct example5_base_grammar 
  : qi::grammar<Iterator, qi::in_state_skipper<Lexer> >
{
    template <typename TokenDef>
    example5_base_grammar(TokenDef const& tok)
      : example5_base_grammar::base_type(program)
    {
        using boost::spirit::_val;

        program 
            =  +block
            ;

        block
            =   '{' >> *statement >> '}'
            ;

        statement 
            =   assignment
            |   if_stmt
            |   while_stmt
            ;

        assignment 
            =   (tok.identifier >> '=' >> expression >> ';')
                [
                    std::cout << val("assignment statement to: ") << _1 << "\n"
                ]
            ;

        if_stmt
            =   (tok.if_ >> '(' >> expression >> ')' >> block)
                [
                    std::cout << val("if expression: ") << _1 << "\n"
                ]
            ;

        while_stmt 
            =   (tok.while_ >> '(' >> expression >> ')' >> block)
                [
                    std::cout << val("while expression: ") << _1 << "\n"
                ]
            ;

        //  since expression has a variant return type accommodating for 
        //  std::string and unsigned integer, both possible values may be 
        //  returned to the calling rule
        expression 
            =   tok.identifier [ _val = _1 ]
            |   tok.constant   [ _val = _1 ]
            ;
    }

    typedef qi::in_state_skipper<Lexer> skipper_type;

    qi::rule<Iterator, skipper_type> program, block, statement;
    qi::rule<Iterator, skipper_type> assignment, if_stmt;
    qi::rule<Iterator, skipper_type> while_stmt;

    //  the expression is the only rule having a return value
    typedef boost::variant<unsigned int, std::string> expression_type;
    qi::rule<Iterator, expression_type(), skipper_type>  expression;
};

///////////////////////////////////////////////////////////////////////////////
//  Token definition for derived lexer, defines additional tokens 
///////////////////////////////////////////////////////////////////////////////
template <typename Lexer>
struct example5_tokens : example5_base_tokens<Lexer>
{
    typedef example5_base_tokens<Lexer> base_type;

    example5_tokens()
    {
        // define the additional token to match
        else_ = "else";

        // associate the new token with the lexer, note we add 'else' before 
        // anything else to add it to the token set before the identifier 
        // token, otherwise "else" would be matched as an identifier
        this->self = else_;

        // now add the token definitions from the base class
        this->base_type::init_token_definitions();
    }

    // this token has no attribute
    lex::token_def<lex::omit> else_;
};

///////////////////////////////////////////////////////////////////////////////
//  Derived grammar definition, defines a language extension
///////////////////////////////////////////////////////////////////////////////
template <typename Iterator, typename Lexer>
struct example5_grammar : example5_base_grammar<Iterator, Lexer>
{
    template <typename TokenDef>
    example5_grammar(TokenDef const& tok)
      : example5_base_grammar<Iterator, Lexer>(tok)
    {
        // we alter the if_stmt only
        this->if_stmt
            =   this->if_stmt.copy() >> -(tok.else_ >> this->block)
            ;
    }
};

///////////////////////////////////////////////////////////////////////////////
int main()
{
    // iterator type used to expose the underlying input stream
    typedef std::string::iterator base_iterator_type;

    // This is the lexer token type to use. The second template parameter lists 
    // all attribute types used for token_def's during token definition (see 
    // example5_base_tokens<> above). Here we use the predefined lexertl token 
    // type, but any compatible token type may be used instead.
    //
    // If you don't list any token attribute types in the following declaration 
    // (or just use the default token type: lexertl_token<base_iterator_type>)  
    // it will compile and work just fine, just a bit less efficient. This is  
    // because the token attribute will be generated from the matched input  
    // sequence every time it is requested. But as soon as you specify at 
    // least one token attribute type you'll have to list all attribute types 
    // used for token_def<> declarations in the token definition class above,  
    // otherwise compilation errors will occur.
    typedef lex::lexertl::token<
        base_iterator_type, boost::mpl::vector<unsigned int, std::string> 
    > token_type;

    // Here we use the lexertl based lexer engine.
    typedef lex::lexertl::lexer<token_type> lexer_type;

    // This is the token definition type (derived from the given lexer type).
    typedef example5_tokens<lexer_type> example5_tokens;

    // this is the iterator type exposed by the lexer 
    typedef example5_tokens::iterator_type iterator_type;

    // this is the type of the grammar to parse
    typedef example5_grammar<iterator_type, example5_tokens::lexer_def> example5_grammar;

    // now we use the types defined above to create the lexer and grammar
    // object instances needed to invoke the parsing process
    example5_tokens tokens;                         // Our lexer
    example5_grammar calc(tokens);                  // Our parser

    std::string str (read_from_file("example5.input"));

    // At this point we generate the iterator pair used to expose the
    // tokenized input stream.
    std::string::iterator it = str.begin();
    iterator_type iter = tokens.begin(it, str.end());
    iterator_type end = tokens.end();

    // Parsing is done based on the the token stream, not the character 
    // stream read from the input.
    // Note how we use the lexer defined above as the skip parser. It must
    // be explicitly wrapped inside a state directive, switching the lexer 
    // state for the duration of skipping whitespace.
    std::string ws("WS");
    bool r = qi::phrase_parse(iter, end, calc, qi::in_state(ws)[tokens.self]);

    if (r && iter == end)
    {
        std::cout << "-------------------------\n";
        std::cout << "Parsing succeeded\n";
        std::cout << "-------------------------\n";
    }
    else
    {
        std::cout << "-------------------------\n";
        std::cout << "Parsing failed\n";
        std::cout << "-------------------------\n";
    }

    std::cout << "Bye... :-) \n\n";
    return 0;
}
