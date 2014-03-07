//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// #define BOOST_SPIRIT_LEXERTL_DEBUG

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/foreach.hpp>

using namespace boost::spirit;

///////////////////////////////////////////////////////////////////////////////
// semantic action analyzing leading whitespace 
enum tokenids
{
    ID_INDENT = 1000,
    ID_DEDENT
};

struct handle_whitespace
{
    handle_whitespace(std::stack<unsigned int>& indents)
      : indents_(indents) {}

    template <typename Iterator, typename IdType, typename Context>
    void operator()(Iterator& start, Iterator& end
      , BOOST_SCOPED_ENUM(lex::pass_flags)& pass, IdType& id
      , Context& ctx)
    {
        unsigned int level = 0;
        if (is_indent(start, end, level)) {
            id = ID_INDENT;
            ctx.set_value(level);
        }
        else if (is_dedent(start, end, level)) {
            id = ID_DEDENT;
            ctx.set_value(level);
        }
        else {
            pass = lex::pass_flags::pass_ignore;
        }
    }

    // Get indentation level, for now (no tabs) we just count the spaces
    // once we allow tabs in the regex this needs to be expanded
    template <typename Iterator>
    unsigned int get_indent(Iterator& start, Iterator& end)
    {
        return std::distance(start, end);
    }

    template <typename Iterator>
    bool is_dedent(Iterator& start, Iterator& end, unsigned int& level) 
    {
        unsigned int newindent = get_indent(start, end);
        while (!indents_.empty() && newindent < indents_.top()) {
            level++;        // dedent one more level
            indents_.pop();
        }
        return level > 0;
    }

    // Handle additional indentation
    template <typename Iterator>
    bool is_indent(Iterator& start, Iterator& end, unsigned int& level) 
    {
        unsigned int newindent = get_indent(start, end);
        if (indents_.empty() || newindent > indents_.top()) {
            level = 1;      // indent one more level 
            indents_.push(newindent);
            return true;
        }
        return false;
    }

    std::stack<unsigned int>& indents_;

private:
    // silence MSVC warning C4512: assignment operator could not be generated
    handle_whitespace& operator= (handle_whitespace const&);
};

///////////////////////////////////////////////////////////////////////////////
//  Token definition
template <typename Lexer>
struct set_token_value : boost::spirit::lex::lexer<Lexer>
{
    set_token_value()
    {
        using lex::_pass;

        // define tokens and associate them with the lexer
        whitespace = "^[ ]*";
        newline = '\n';

        this->self = whitespace[ handle_whitespace(indents) ];
        this->self += newline[ _pass = lex::pass_flags::pass_ignore ];
    }

    lex::token_def<unsigned int> whitespace;
    lex::token_def<> newline;
    std::stack<unsigned int> indents;
};

///////////////////////////////////////////////////////////////////////////////
struct token_data
{
    int id;
    unsigned int value;
};

template <typename Token>
inline 
bool test_tokens(token_data const* d, std::vector<Token> const& tokens)
{
    BOOST_FOREACH(Token const& t, tokens)
    {
        if (d->id == -1)
            return false;           // reached end of expected data

        typename Token::token_value_type const& value (t.value());
        if (t.id() != static_cast<std::size_t>(d->id))        // token id must match
            return false;
        if (value.which() != 1)     // must have an integer value 
            return false;
        if (boost::get<unsigned int>(value) != d->value)  // value must match
            return false;
        ++d;
    }

    return (d->id == -1) ? true : false;
}

inline 
bool test_indents(int *i, std::stack<unsigned int>& indents)
{
    while (!indents.empty())
    {
        if (*i == -1)
            return false;           // reached end of expected data
        if (indents.top() != static_cast<unsigned int>(*i))
            return false;           // value must match

        ++i;
        indents.pop();
    }

    return (*i == -1) ? true : false;
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    namespace lex = boost::spirit::lex;
    namespace phoenix = boost::phoenix;

    typedef std::string::iterator base_iterator_type;
    typedef boost::mpl::vector<unsigned int> token_value_types;
    typedef lex::lexertl::token<base_iterator_type, token_value_types> token_type;
    typedef lex::lexertl::actor_lexer<token_type> lexer_type;

    // test simple indent
    {
        set_token_value<lexer_type> lexer;
        std::vector<token_type> tokens;
        std::string input("    ");
        base_iterator_type first = input.begin();

        using phoenix::arg_names::_1;
        BOOST_TEST(lex::tokenize(first, input.end(), lexer
          , phoenix::push_back(phoenix::ref(tokens), _1)));

        int i[] = { 4, -1 };
        BOOST_TEST(test_indents(i, lexer.indents));

        token_data d[] = { { ID_INDENT, 1 }, { -1, 0 } };
        BOOST_TEST(test_tokens(d, tokens));
    }

    // test two indents
    {
        set_token_value<lexer_type> lexer;
        std::vector<token_type> tokens;
        std::string input(
            "    \n"
            "        \n");
        base_iterator_type first = input.begin();

        using phoenix::arg_names::_1;
        BOOST_TEST(lex::tokenize(first, input.end(), lexer
          , phoenix::push_back(phoenix::ref(tokens), _1)));

        int i[] = { 8, 4, -1 };
        BOOST_TEST(test_indents(i, lexer.indents));

        token_data d[] = { 
            { ID_INDENT, 1 }, { ID_INDENT, 1 }
          , { -1, 0 } };
        BOOST_TEST(test_tokens(d, tokens));
    }

    // test one dedent
    {
        set_token_value<lexer_type> lexer;
        std::vector<token_type> tokens;
        std::string input(
            "    \n"
            "        \n"
            "    \n");
        base_iterator_type first = input.begin();

        using phoenix::arg_names::_1;
        BOOST_TEST(lex::tokenize(first, input.end(), lexer
          , phoenix::push_back(phoenix::ref(tokens), _1)));

        int i[] = { 4, -1 };
        BOOST_TEST(test_indents(i, lexer.indents));

        token_data d[] = { 
            { ID_INDENT, 1 }, { ID_INDENT, 1 }
          , { ID_DEDENT, 1 }
          , { -1, 0 } };
        BOOST_TEST(test_tokens(d, tokens));
    }

    // test two dedents
    {
        set_token_value<lexer_type> lexer;
        std::vector<token_type> tokens;
        std::string input(
            "    \n"
            "        \n"
            "            \n"
            "    \n");
        base_iterator_type first = input.begin();

        using phoenix::arg_names::_1;
        BOOST_TEST(lex::tokenize(first, input.end(), lexer
          , phoenix::push_back(phoenix::ref(tokens), _1)));

        int i[] = { 4, -1 };
        BOOST_TEST(test_indents(i, lexer.indents));

        token_data d[] = { 
            { ID_INDENT, 1 }, { ID_INDENT, 1 }, { ID_INDENT, 1 }
          , { ID_DEDENT, 2 }
          , { -1, 0 } };
        BOOST_TEST(test_tokens(d, tokens));
    }

    return boost::report_errors();
}

