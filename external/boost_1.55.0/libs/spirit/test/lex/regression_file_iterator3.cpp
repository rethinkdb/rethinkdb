//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2010 Mathias Gaunard
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_SPIRIT_DEBUG 1    // required for token streaming
// #define BOOST_SPIRIT_LEXERTL_DEBUG 1

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/support_multi_pass.hpp>
#include <boost/spirit/include/classic_position_iterator.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>

#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>

#include <sstream>

namespace spirit = boost::spirit;
namespace lex = spirit::lex;
namespace phoenix = boost::phoenix;

typedef spirit::classic::position_iterator2<
    spirit::multi_pass<std::istreambuf_iterator<char> >
> file_iterator;

typedef boost::iterator_range<file_iterator> file_range;

inline file_iterator 
make_file_iterator(std::istream& input, const std::string& filename)
{
    return file_iterator(
        spirit::make_default_multi_pass(
            std::istreambuf_iterator<char>(input)),
        spirit::multi_pass<std::istreambuf_iterator<char> >(),
        filename);
}

struct string_literal
{
    string_literal(file_iterator, file_iterator)
    {
    }
};

typedef lex::lexertl::token<
    file_iterator, boost::mpl::vector<string_literal>
> token_type;

struct lexer
  : lex::lexer<lex::lexertl::actor_lexer<token_type> >
{
    lexer() : st("'[^'\\n]*'", 1)
    {
        lex::token_def<> string_lookahead('\'');
        self("LA") = string_lookahead;

        // make sure lookahead is implicitly evaluated using the lexer state
        // the token_def has been associated with
        self = st [
                phoenix::if_(lex::lookahead(string_lookahead)) [ lex::more() ]
            ]
            ;
    }
    
    lex::token_def<string_literal> st;
};

typedef lexer::iterator_type token_iterator;

int main()
{
    std::stringstream ss;
    ss << "'foo''bar'";

    file_iterator begin = make_file_iterator(ss, "SS");
    file_iterator end;

    lexer l;
    token_iterator begin2 = l.begin(begin, end);
    token_iterator end2 = l.end();

    char const* test_data[] = { "1,'foo'", "1,'foo''bar'" };
    std::size_t const test_data_size = sizeof(test_data)/sizeof(test_data[0]);

    token_iterator it = begin2;
    std::size_t i = 0;
    for (/**/; it != end2 && i < test_data_size; ++it, ++i)
    {
        std::stringstream ss;
        ss << it->id() << "," << *it;
        BOOST_TEST(ss.str() == test_data[i]);
    }
    BOOST_TEST(it == end2);
    BOOST_TEST(i == test_data_size);

    return boost::report_errors();
}
