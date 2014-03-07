//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// #define BOOST_SPIRIT_LEXERTL_DEBUG

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/lex_lexertl_position_token.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/qi_numeric.hpp>

namespace spirit = boost::spirit;
namespace lex = boost::spirit::lex;
namespace phoenix = boost::phoenix;
namespace mpl = boost::mpl;

///////////////////////////////////////////////////////////////////////////////
enum tokenids
{
    ID_INT = 1000,
    ID_DOUBLE
};

template <typename Lexer>
struct token_definitions : lex::lexer<Lexer>
{
    token_definitions()
    {
        this->self.add_pattern("HEXDIGIT", "[0-9a-fA-F]");
        this->self.add_pattern("OCTALDIGIT", "[0-7]");
        this->self.add_pattern("DIGIT", "[0-9]");

        this->self.add_pattern("OPTSIGN", "[-+]?");
        this->self.add_pattern("EXPSTART", "[eE][-+]");
        this->self.add_pattern("EXPONENT", "[eE]{OPTSIGN}{DIGIT}+");

        // define tokens and associate them with the lexer
        int_ = "(0x|0X){HEXDIGIT}+|0{OCTALDIGIT}*|{OPTSIGN}[1-9]{DIGIT}*";
        int_.id(ID_INT);

        double_ = "{OPTSIGN}({DIGIT}*\\.{DIGIT}+|{DIGIT}+\\.){EXPONENT}?|{DIGIT}+{EXPONENT}";
        double_.id(ID_DOUBLE);

        whitespace = "[ \t\n]+";

        this->self = 
                double_ 
            |   int_ 
            |   whitespace[ lex::_pass = lex::pass_flags::pass_ignore ]
            ;
    }

    lex::token_def<int> int_;
    lex::token_def<double> double_;
    lex::token_def<lex::omit> whitespace;
};

template <typename Lexer>
struct token_definitions_with_state : lex::lexer<Lexer>
{
    token_definitions_with_state()
    {
        this->self.add_pattern("HEXDIGIT", "[0-9a-fA-F]");
        this->self.add_pattern("OCTALDIGIT", "[0-7]");
        this->self.add_pattern("DIGIT", "[0-9]");

        this->self.add_pattern("OPTSIGN", "[-+]?");
        this->self.add_pattern("EXPSTART", "[eE][-+]");
        this->self.add_pattern("EXPONENT", "[eE]{OPTSIGN}{DIGIT}+");

        this->self.add_state();
        this->self.add_state("INT");
        this->self.add_state("DOUBLE");

        // define tokens and associate them with the lexer
        int_ = "(0x|0X){HEXDIGIT}+|0{OCTALDIGIT}*|{OPTSIGN}[1-9]{DIGIT}*";
        int_.id(ID_INT);

        double_ = "{OPTSIGN}({DIGIT}*\\.{DIGIT}+|{DIGIT}+\\.){EXPONENT}?|{DIGIT}+{EXPONENT}";
        double_.id(ID_DOUBLE);

        whitespace = "[ \t\n]+";

        this->self("*") = 
                double_ [ lex::_state = "DOUBLE"] 
            |   int_ [ lex::_state = "INT" ]
            |   whitespace[ lex::_pass = lex::pass_flags::pass_ignore ]
            ;
    }

    lex::token_def<int> int_;
    lex::token_def<double> double_;
    lex::token_def<lex::omit> whitespace;
};

///////////////////////////////////////////////////////////////////////////////
template <typename Token>
inline bool 
test_token_ids(int const* ids, std::vector<Token> const& tokens)
{
    BOOST_FOREACH(Token const& t, tokens)
    {
        if (*ids == -1)
            return false;           // reached end of expected data

        if (t.id() != static_cast<std::size_t>(*ids))        // token id must match
            return false;

        ++ids;
    }

    return (*ids == -1) ? true : false;
}

///////////////////////////////////////////////////////////////////////////////
template <typename Token>
inline bool 
test_token_states(std::size_t const* states, std::vector<Token> const& tokens)
{
    BOOST_FOREACH(Token const& t, tokens)
    {
        if (*states == std::size_t(-1))
            return false;           // reached end of expected data

        if (t.state() != *states)            // token state must match
            return false;

        ++states;
    }

    return (*states == std::size_t(-1)) ? true : false;
}

///////////////////////////////////////////////////////////////////////////////
struct position_type
{
    std::size_t begin, end;
};

template <typename Iterator, typename Token>
inline bool 
test_token_positions(Iterator begin, position_type const* positions, 
    std::vector<Token> const& tokens)
{
    BOOST_FOREACH(Token const& t, tokens)
    {
        if (positions->begin == std::size_t(-1) && 
            positions->end == std::size_t(-1))
        {
            return false;           // reached end of expected data
        }

        boost::iterator_range<Iterator> matched = t.matched();
        std::size_t start = std::distance(begin, matched.begin());
        std::size_t end = std::distance(begin, matched.end());

        // position must match
        if (start != positions->begin || end != positions->end)
            return false;

        ++positions;
    }

    return (positions->begin == std::size_t(-1) && 
            positions->end == std::size_t(-1)) ? true : false;
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
struct value 
{
    bool valid;
    T val;
};

template <typename T, typename Token>
inline bool 
test_token_values(value<T> const* values, std::vector<Token> const& tokens)
{
    BOOST_FOREACH(Token const& t, tokens)
    {
        if (values->valid && values->val == 0)
            return false;               // reached end of expected data

        if (values->valid) {
            T val;
            spirit::traits::assign_to(t, val);
            if (val != values->val)     // token value must match
                return false;
        }

        ++values;
    }

    return (values->valid && values->val == 0) ? true : false;
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    typedef std::string::iterator base_iterator_type;
    std::string input(" 01 1.2 -2 03  2.3e6 -3.4");
    int ids[] = { ID_INT, ID_DOUBLE, ID_INT, ID_INT, ID_DOUBLE, ID_DOUBLE, -1 };
    std::size_t states[] = { 0, 1, 2, 1, 1, 2, std::size_t(-1) };
    position_type positions[] = 
    {
        { 1, 3 }, { 4, 7 }, { 8, 10 }, { 11, 13 }, { 15, 20 }, { 21, 25 }, 
        { std::size_t(-1), std::size_t(-1) }
    };
    value<int> ivalues[] = { 
        { true, 1 }, { false }, { true, -2 }, 
        { true, 3 }, { false }, { false }, 
        { true, 0 }
    };
    value<double> dvalues[] = { 
        { false }, { true, 1.2 }, { false }, 
        { false }, { true, 2.3e6 }, { true, -3.4 }, 
        { true, 0.0 }
    };

    // token type: token id, iterator_pair as token value, no state
    {
        typedef lex::lexertl::token<
            base_iterator_type, mpl::vector<double, int>, mpl::false_> token_type;
        typedef lex::lexertl::actor_lexer<token_type> lexer_type;

        token_definitions<lexer_type> lexer;
        std::vector<token_type> tokens;
        base_iterator_type first = input.begin();

        using phoenix::arg_names::_1;
        BOOST_TEST(lex::tokenize(first, input.end(), lexer
          , phoenix::push_back(phoenix::ref(tokens), _1)));

        BOOST_TEST(test_token_ids(ids, tokens));
        BOOST_TEST(test_token_values(ivalues, tokens));
        BOOST_TEST(test_token_values(dvalues, tokens));
    }

    {
        typedef lex::lexertl::position_token<
            base_iterator_type, mpl::vector<double, int>, mpl::false_> token_type;
        typedef lex::lexertl::actor_lexer<token_type> lexer_type;

        token_definitions<lexer_type> lexer;
        std::vector<token_type> tokens;
        base_iterator_type first = input.begin();

        using phoenix::arg_names::_1;
        BOOST_TEST(lex::tokenize(first, input.end(), lexer
          , phoenix::push_back(phoenix::ref(tokens), _1)));

        BOOST_TEST(test_token_ids(ids, tokens));
        BOOST_TEST(test_token_positions(input.begin(), positions, tokens));
        BOOST_TEST(test_token_values(ivalues, tokens));
        BOOST_TEST(test_token_values(dvalues, tokens));
    }

    // token type: holds token id, state, iterator_pair as token value
    {
        typedef lex::lexertl::token<
            base_iterator_type, mpl::vector<double, int>, mpl::true_> token_type;
        typedef lex::lexertl::actor_lexer<token_type> lexer_type;

        token_definitions_with_state<lexer_type> lexer;
        std::vector<token_type> tokens;
        base_iterator_type first = input.begin();

        using phoenix::arg_names::_1;
        BOOST_TEST(lex::tokenize(first, input.end(), lexer
          , phoenix::push_back(phoenix::ref(tokens), _1)));

        BOOST_TEST(test_token_ids(ids, tokens));
        BOOST_TEST(test_token_states(states, tokens));
        BOOST_TEST(test_token_values(ivalues, tokens));
        BOOST_TEST(test_token_values(dvalues, tokens));
    }

    {
        typedef lex::lexertl::position_token<
            base_iterator_type, mpl::vector<double, int>, mpl::true_> token_type;
        typedef lex::lexertl::actor_lexer<token_type> lexer_type;

        token_definitions_with_state<lexer_type> lexer;
        std::vector<token_type> tokens;
        base_iterator_type first = input.begin();

        using phoenix::arg_names::_1;
        BOOST_TEST(lex::tokenize(first, input.end(), lexer
          , phoenix::push_back(phoenix::ref(tokens), _1)));

        BOOST_TEST(test_token_ids(ids, tokens));
        BOOST_TEST(test_token_states(states, tokens));
        BOOST_TEST(test_token_positions(input.begin(), positions, tokens));
        BOOST_TEST(test_token_values(ivalues, tokens));
        BOOST_TEST(test_token_values(dvalues, tokens));
    }

    return boost::report_errors();
}
