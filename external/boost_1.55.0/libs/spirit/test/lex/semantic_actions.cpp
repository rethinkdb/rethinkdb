//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/lightweight_test.hpp>
#include <boost/config/warning_disable.hpp>

#include <boost/spirit/include/lex_lexertl.hpp>

namespace lex = boost::spirit::lex;

typedef lex::lexertl::token<std::string::iterator> token_type;
typedef lex::lexertl::actor_lexer<token_type> lexer_type;

///////////////////////////////////////////////////////////////////////////////
static bool found_identifier_flag = false;

///////////////////////////////////////////////////////////////////////////////
void found_identifier_sa0()
{
    found_identifier_flag = true;
}

template <typename Lexer>
struct lexer_sa0 : lex::lexer<Lexer>
{
    lexer_sa0() 
    {
        identifier = "[a-zA-Z][_a-zA-Z0-9]*";
        this->self += identifier [&found_identifier_sa0];
    }
    lex::token_def<> identifier;
};

///////////////////////////////////////////////////////////////////////////////
static std::string found_identifier_str;

void found_identifier_sa2(std::string::iterator& start
  , std::string::iterator& end)
{
    found_identifier_flag = true;
    found_identifier_str = std::string(start, end);
}

template <typename Lexer>
struct lexer_sa2 : lex::lexer<Lexer>
{
    lexer_sa2() 
    {
        identifier = "[a-zA-Z][_a-zA-Z0-9]*";
        this->self += identifier [&found_identifier_sa2];
    }
    lex::token_def<> identifier;
};

///////////////////////////////////////////////////////////////////////////////
void found_identifier_sa3_normal(std::string::iterator& start
  , std::string::iterator& end, BOOST_SCOPED_ENUM(lex::pass_flags)& pass)
{
    BOOST_TEST(pass == lex::pass_flags::pass_normal);

    found_identifier_flag = true;
    found_identifier_str = std::string(start, end);
}

template <typename Lexer>
struct lexer_sa3_normal : lex::lexer<Lexer>
{
    lexer_sa3_normal() 
    {
        identifier = "[a-zA-Z][_a-zA-Z0-9]*";
        this->self += identifier [&found_identifier_sa3_normal];
    }
    lex::token_def<> identifier;
};

void found_identifier_sa3_fail(std::string::iterator&, std::string::iterator&
  , BOOST_SCOPED_ENUM(lex::pass_flags)& pass)
{
    pass = lex::pass_flags::pass_fail;
}

template <typename Lexer>
struct lexer_sa3_fail : lex::lexer<Lexer>
{
    lexer_sa3_fail() 
    {
        identifier = "[a-zA-Z][_a-zA-Z0-9]*";
        this->self += identifier [&found_identifier_sa3_fail];
    }
    lex::token_def<> identifier;
};

void found_identifier_sa3_ignore(std::string::iterator&, std::string::iterator&
  , BOOST_SCOPED_ENUM(lex::pass_flags)& pass)
{
    pass = lex::pass_flags::pass_ignore;
}

template <typename Lexer>
struct lexer_sa3_ignore : lex::lexer<Lexer>
{
    lexer_sa3_ignore() 
    {
        identifier = "[a-zA-Z][_a-zA-Z0-9]*";
        this->self += identifier [&found_identifier_sa3_ignore];
    }
    lex::token_def<> identifier;
};

///////////////////////////////////////////////////////////////////////////////
static std::size_t found_identifier_id = 0;

void found_identifier_sa4(std::string::iterator& start
  , std::string::iterator& end, BOOST_SCOPED_ENUM(lex::pass_flags)& pass
  , std::size_t id)
{
    BOOST_TEST(pass == lex::pass_flags::pass_normal);

    found_identifier_flag = true;
    found_identifier_str = std::string(start, end);
    found_identifier_id = id;
}

template <typename Lexer>
struct lexer_sa4 : lex::lexer<Lexer>
{
    lexer_sa4() 
    {
        identifier = "[a-zA-Z][_a-zA-Z0-9]*";
        this->self += identifier [&found_identifier_sa4];
    }
    lex::token_def<> identifier;
};

void found_identifier_sa4_id(std::string::iterator& start
  , std::string::iterator& end, BOOST_SCOPED_ENUM(lex::pass_flags)& pass
  , std::size_t& id)
{
    BOOST_TEST(pass == lex::pass_flags::pass_normal);

    found_identifier_flag = true;
    found_identifier_str = std::string(start, end);
    found_identifier_id = id;
    id = 1;
}

template <typename Lexer>
struct lexer_sa4_id : lex::lexer<Lexer>
{
    lexer_sa4_id() 
    {
        identifier = "[a-zA-Z][_a-zA-Z0-9]*";
        this->self += identifier [&found_identifier_sa4_id];
    }
    lex::token_def<> identifier;
};

static std::size_t found_identifier_id2 = 0;

bool identifier_token(token_type const& t)
{
    found_identifier_id2 = t.id();
    return true;
}

///////////////////////////////////////////////////////////////////////////////
struct found_identifier_sa5
{
    template <typename Context>
    void operator()(std::string::iterator& /*start*/
      , std::string::iterator& /*end*/, BOOST_SCOPED_ENUM(lex::pass_flags)& pass
      , std::size_t& /*id*/, Context& ctx)
    {
        BOOST_TEST(pass == lex::pass_flags::pass_normal);

        found_identifier_flag = true;
        found_identifier_str = std::string(ctx.get_value().begin(), ctx.get_value().end());
    }
};

template <typename Lexer>
struct lexer_sa5 : lex::lexer<Lexer>
{
    lexer_sa5() 
    {
        identifier = "[a-zA-Z][_a-zA-Z0-9]*";
        this->self += identifier [found_identifier_sa5()];
    }
    lex::token_def<> identifier;
};

///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::string identifier ("id_1234");
    std::string::iterator first = identifier.begin();
    std::string::iterator last = identifier.end();

    // test semantic action taking no arguments
    found_identifier_flag = false;
    {
        lexer_sa0<lexer_type> sa0;
        BOOST_TEST(lex::tokenize(first, last, sa0));
        BOOST_TEST(first == last);
        BOOST_TEST(found_identifier_flag);
    }

    // test semantic action taking two arguments (iterator pair for matched 
    // sequence)
    found_identifier_flag = false;
    found_identifier_str.clear();
    first = identifier.begin();
    {
        lexer_sa2<lexer_type> sa2;
        BOOST_TEST(lex::tokenize(first, last, sa2));
        BOOST_TEST(first == last);
        BOOST_TEST(found_identifier_flag);
        BOOST_TEST(found_identifier_str == identifier);
    }

    // test semantic action taking three arguments (iterator pair for matched 
    // sequence and pass_flags) - pass_flags::pass_normal
    found_identifier_flag = false;
    found_identifier_str.clear();
    first = identifier.begin();
    {
        lexer_sa3_normal<lexer_type> sa3;
        BOOST_TEST(lex::tokenize(first, last, sa3));
        BOOST_TEST(first == last);
        BOOST_TEST(found_identifier_flag);
        BOOST_TEST(found_identifier_str == identifier);
    }

    // test semantic action taking three arguments (iterator pair for matched 
    // sequence and pass_flags) - pass_flags::pass_fail
    first = identifier.begin();
    {
        lexer_sa3_fail<lexer_type> sa3;
        BOOST_TEST(!lex::tokenize(first, last, sa3));
        BOOST_TEST(first != last);
    }

    // test semantic action taking three arguments (iterator pair for matched 
    // sequence and pass_flags) - pass_flags::pass_ignore
    first = identifier.begin();
    {
        lexer_sa3_ignore<lexer_type> sa3;
        BOOST_TEST(lex::tokenize(first, last, sa3));
        BOOST_TEST(first == last);
    }

    // test semantic action taking four arguments (iterator pair for matched 
    // sequence and pass_flags, and token id)
    found_identifier_flag = false;
    found_identifier_str.clear();
    first = identifier.begin();
    found_identifier_id = 0;
    {
        lexer_sa4<lexer_type> sa4;
        BOOST_TEST(lex::tokenize(first, last, sa4));
        BOOST_TEST(first == last);
        BOOST_TEST(found_identifier_flag);
        BOOST_TEST(found_identifier_str == identifier);
        BOOST_TEST(found_identifier_id == lex::min_token_id);
    }

    found_identifier_flag = false;
    found_identifier_str.clear();
    first = identifier.begin();
    found_identifier_id = 0;
    found_identifier_id2 = 0;
    {
        lexer_sa4_id<lexer_type> sa4;
        BOOST_TEST(lex::tokenize(first, last, sa4, identifier_token));
        BOOST_TEST(first == last);
        BOOST_TEST(found_identifier_flag);
        BOOST_TEST(found_identifier_str == identifier);
        BOOST_TEST(found_identifier_id == lex::min_token_id);
        BOOST_TEST(found_identifier_id2 == 1);
    }

    // test semantic action taking four arguments (iterator pair for matched 
    // sequence and pass_flags, token id, and context)
    found_identifier_flag = false;
    found_identifier_str.clear();
    first = identifier.begin();
    found_identifier_id = 0;
    found_identifier_id2 = 0;
    {
        lexer_sa5<lexer_type> sa5;
        BOOST_TEST(lex::tokenize(first, last, sa5));
        BOOST_TEST(first == last);
        BOOST_TEST(found_identifier_flag);
        BOOST_TEST(found_identifier_str == identifier);
    }

    return boost::report_errors();
}
