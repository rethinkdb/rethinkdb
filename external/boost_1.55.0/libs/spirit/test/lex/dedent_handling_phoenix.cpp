//  Copyright (c) 2009 Carl Barron
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <sstream>

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/lex.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/phoenix.hpp>

namespace lex = boost::spirit::lex;
namespace phoenix = boost::phoenix;

///////////////////////////////////////////////////////////////////////////////
template <typename Lexer>
struct multi_tokens : lex::lexer<Lexer>
{
    int level;

    multi_tokens() : level(0)
    {
        using lex::_state;
        using lex::_start;
        using lex::_end;
        using lex::_pass;
        using lex::pass_flags;

        a = "A";
        b = "B";
        c = "C";
        this->self = 
                a [ ++phoenix::ref(level) ]
            |   b
            |   c [
                      _state = "in_dedenting",
                      _end = _start,
                      _pass = pass_flags::pass_ignore
                  ]
            ;

        d = ".";
        this->self("in_dedenting") = 
                d [ 
                      if_(--phoenix::ref(level)) [ 
                          _end = _start 
                      ]
                      .else_ [ 
                          _state = "INITIAL"
                      ]
                  ]
            ;
    }

    lex::token_def<> a, b, c, d;
};

struct dumper
{
    typedef bool result_type;

    dumper(std::stringstream& strm) : strm(strm) {}

    template <typename Token>
    bool operator () (Token const &t)
    {
        strm << (char)(t.id() - lex::min_token_id + 'a');
        return true;
    }

    std::stringstream& strm;

private:
    // silence MSVC warning C4512: assignment operator could not be generated
    dumper& operator= (dumper const&);
};

///////////////////////////////////////////////////////////////////////////////
int main()
{
    typedef lex::lexertl::token<std::string::iterator> token_type;
    typedef lex::lexertl::actor_lexer<token_type> base_lexer_type;
    typedef multi_tokens<base_lexer_type> lexer_type;
    typedef lexer_type::iterator_type iterator;

    std::string in("AAABBC");
    std::string::iterator first(in.begin());
    std::stringstream strm;

    lexer_type the_lexer;
    BOOST_TEST(lex::tokenize(first, in.end(), the_lexer, dumper(strm)));
    BOOST_TEST(strm.str() == "aaabbddd");

    return boost::report_errors();
}

