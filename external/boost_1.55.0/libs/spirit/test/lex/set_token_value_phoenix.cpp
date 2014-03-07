//  Copyright (c) 2009 Carl Barron
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <string>
#include <iostream>

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/lex.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/phoenix.hpp>

namespace lex = boost::spirit::lex;
namespace phoenix = boost::phoenix;

///////////////////////////////////////////////////////////////////////////////
struct square_impl
{
    template <class>
    struct result { typedef int type; };

    template <class A>
    int operator () (const A &x) const
    { return (x) * (x); }
};

phoenix::function<square_impl> const square = square_impl();

///////////////////////////////////////////////////////////////////////////////
template <class Lexer>
struct test_tokens : lex::lexer<Lexer>
{
    test_tokens()
    {
        a = "a";
        this->self = a [lex::_val = square(*lex::_start)];
    }

    lex::token_def<int> a;
};

struct catch_result
{
    template <class Token>
    bool operator() (Token const& x) const
    {
        BOOST_TEST(x.value().which() == 1);
        BOOST_TEST(boost::get<int>(x.value()) == 9409);  // 9409 == 'a' * 'a'
        return true;
    }
};

///////////////////////////////////////////////////////////////////////////////
int main()
{
    typedef lex::lexertl::token<std::string::iterator
      , boost::mpl::vector<int> > token_type;

    std::string in = "a";
    std::string::iterator first(in.begin());

    test_tokens<lex::lexertl::actor_lexer<token_type> > the_lexer;
    BOOST_TEST(lex::tokenize(first, in.end(), the_lexer, catch_result()));

    return boost::report_errors();
}
