//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// #define BOOST_SPIRIT_LEXERTL_DEBUG 1

#include <boost/config/warning_disable.hpp>

#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

namespace lex = boost::spirit::lex;
namespace qi = boost::spirit::qi;
namespace phoenix = boost::phoenix;

///////////////////////////////////////////////////////////////////////////////
template <typename Lexer>
struct language_tokens : lex::lexer<Lexer>
{
    language_tokens()
    {
        tok_float = "float";
        tok_int = "int";
        floatlit = "[0-9]+\\.[0-9]*";
        intlit = "[0-9]+";
        ws = "[ \t\n]+";
        identifier = "[a-zA-Z_][a-zA-Z_0-9]*";
      
        this->self = ws [lex::_pass = lex::pass_flags::pass_ignore];
        this->self += tok_float | tok_int | floatlit | intlit | identifier;
        this->self += lex::char_('=');
    }

    lex::token_def<> tok_float, tok_int;
    lex::token_def<> ws;
    lex::token_def<double> floatlit;
    lex::token_def<int> intlit;
    lex::token_def<> identifier;
};

///////////////////////////////////////////////////////////////////////////////
template <typename Iterator>
struct language_grammar : qi::grammar<Iterator>
{
    template <typename Lexer>
    language_grammar(language_tokens<Lexer> const& tok) 
      : language_grammar::base_type(declarations)
    {
        declarations = +number;
        number = 
                tok.tok_float >> tok.identifier >> '=' >> tok.floatlit
            |   tok.tok_int >> tok.identifier >> '=' >> tok.intlit
            ;

        declarations.name("declarations");
        number.name("number");
        debug(declarations);
        debug(number);
    }

    qi::rule<Iterator> declarations;
    qi::rule<Iterator> number;
};

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // iterator type used to expose the underlying input stream
    typedef std::string::iterator base_iterator_type;

    // lexer type
    typedef lex::lexertl::actor_lexer<
        lex::lexertl::token<
            base_iterator_type, boost::mpl::vector2<double, int> 
        > > lexer_type;

    // iterator type exposed by the lexer 
    typedef language_tokens<lexer_type>::iterator_type iterator_type;

    // now we use the types defined above to create the lexer and grammar
    // object instances needed to invoke the parsing process
    language_tokens<lexer_type> tokenizer;           // Our lexer
    language_grammar<iterator_type> g (tokenizer);   // Our parser 

    // Parsing is done based on the the token stream, not the character 
    // stream read from the input.
    std::string str ("float f = 3.4\nint i = 6\n");
    base_iterator_type first = str.begin();

    bool r = lex::tokenize_and_parse(first, str.end(), tokenizer, g);

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
