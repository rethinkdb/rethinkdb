//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2010 Mathias Gaunard
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// #define BOOST_SPIRIT_LEXERTL_DEBUG

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/support_multi_pass.hpp>
#include <boost/spirit/include/classic_position_iterator.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>

#include <boost/spirit/include/phoenix.hpp>

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

struct identifier
{
    identifier(file_iterator, file_iterator)
    {
    }
};

struct string_literal
{
    string_literal(file_iterator, file_iterator)
    {
    }
};

typedef lex::lexertl::token<
    file_iterator, boost::mpl::vector<identifier, string_literal>
> token_type;

struct lexer
  : lex::lexer<lex::lexertl::actor_lexer<token_type> >
{
    lexer() 
      : id("[a-zA-Z0-9]+", 1)
      , st("'[^'\\n]*'", 2)
    {
        self("ST") = 
                st [ lex::_state = "INITIAL" ] 
            ;
        
        self("*") =  
                id                       [ lex::_state = "ST" ]
            |   lex::token_def<>(".", 3) [ lex::_state = "ST" ]
            ;
    }
    
    lex::token_def<identifier> id;
    lex::token_def<string_literal> st;
};

typedef lexer::iterator_type token_iterator;

int main()
{
    std::stringstream ss;
    ss << "foo 'bar'";
    
    file_iterator begin = make_file_iterator(ss, "SS");
    file_iterator end;
    
    lexer l;
    token_iterator begin2 = l.begin(begin, end, "ST");
    token_iterator end2 = l.end();
    
    std::size_t test_data[] = { 1, 3, 2 };
    std::size_t const test_data_size = sizeof(test_data)/sizeof(test_data[0]);

    token_iterator it = begin2;
    std::size_t i = 0;
    for (/**/; it != end2 && i < test_data_size; ++it, ++i)
    {
        BOOST_TEST(it->id() == test_data[i]);
    }
    BOOST_TEST(it == end2);
    BOOST_TEST(i == test_data_size);

    return boost::report_errors();
}
