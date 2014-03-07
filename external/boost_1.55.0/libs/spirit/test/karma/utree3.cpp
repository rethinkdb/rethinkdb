// Copyright (c) 2001-2011 Hartmut Kaiser
// Copyright (c) 2001-2011 Joel de Guzman
// Copyright (c)      2010 Bryce Lelbach
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/mpl/print.hpp>

#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/support_utree.hpp>

#include <sstream>

#include "test.hpp"

int main()
{
    using spirit_test::test;
    using spirit_test::test_delimited;
    using boost::spirit::utree;
    using boost::spirit::utree_type;
    using boost::spirit::utf8_string_range_type;
    using boost::spirit::utf8_string_type;
    using boost::spirit::utf8_symbol_type;

    using boost::spirit::karma::char_;
    using boost::spirit::karma::bool_;
    using boost::spirit::karma::int_;
    using boost::spirit::karma::double_;
    using boost::spirit::karma::string;
    using boost::spirit::karma::space;
    using boost::spirit::karma::rule;

    typedef spirit_test::output_iterator<char>::type output_iterator;

    // as_string
    {   
        using boost::spirit::karma::digit;
        using boost::spirit::karma::as_string;

        utree ut("xy");
        BOOST_TEST(test("xy", string, ut));
        BOOST_TEST(test("xy", as_string[*char_], ut));
        BOOST_TEST(test("x,y", as_string[char_ << ',' << char_], ut));

        ut.clear();
        ut.push_back("ab");
        ut.push_back(1.2);
        BOOST_TEST(test("ab1.2", as_string[*~digit] << double_, ut));
        BOOST_TEST(test("a,b1.2", as_string[~digit % ','] << double_, ut));
    }
    
    // as
    {
        using boost::spirit::karma::digit;
        using boost::spirit::karma::as;
        
        typedef as<std::string> as_string_type;
        as_string_type const as_string = as_string_type();

        typedef as<utf8_symbol_type> as_symbol_type;
        as_symbol_type const as_symbol = as_symbol_type();

        utree ut("xy");
        BOOST_TEST(test("xy", string, ut));
        BOOST_TEST(test("xy", as_string[*char_], ut));
        BOOST_TEST(test("x,y", as_string[char_ << ',' << char_], ut));

        ut.clear();
        ut.push_back("ab");
        ut.push_back(1.2);
        BOOST_TEST(test("ab1.2", as_string[*~digit] << double_, ut));
        BOOST_TEST(test("a,b1.2", as_string[~digit % ','] << double_, ut));
        
        ut = utf8_symbol_type("xy");
        BOOST_TEST(test("xy", string, ut));
        BOOST_TEST(test("xy", as_symbol[*char_], ut));
        BOOST_TEST(test("x,y", as_symbol[char_ << ',' << char_], ut));

        ut.clear();
        ut.push_back(utf8_symbol_type("ab"));
        ut.push_back(1.2);
        BOOST_TEST(test("ab1.2", as_symbol[*~digit] << double_, ut));
        BOOST_TEST(test("a,b1.2", as_symbol[~digit % ','] << double_, ut));
    }
    
    // typed basic_string rules
    {
        utree ut("buzz");

        rule<output_iterator, utf8_string_type()> r1 = string;
        rule<output_iterator, utf8_symbol_type()> r2 = string;

        BOOST_TEST(test("buzz", r1, ut));

        ut = utf8_symbol_type("bar");
        BOOST_TEST(test("bar", r2, ut));
    }

    // parameterized karma::string
    {
        utree ut("foo");

        rule<output_iterator, utf8_string_type()> r1 = string("foo");
        BOOST_TEST(test("foo", string("foo"), ut));
        BOOST_TEST(test("foo", r1, ut));
    }

    {
        using boost::spirit::karma::verbatim;
        using boost::spirit::karma::repeat;
        using boost::spirit::karma::space;
        using boost::spirit::karma::digit;

        utree ut;
        ut.push_back('x');
        ut.push_back('y');
        ut.push_back('c');
        BOOST_TEST(test_delimited("xy c ", verbatim[repeat(2)[char_]] << char_, ut, space));
        BOOST_TEST(test_delimited("x yc ", char_ << verbatim[*char_], ut, space));

        ut.clear();
        ut.push_back('a');
        ut.push_back('b');
        ut.push_back(1.2);
        BOOST_TEST(test_delimited("ab 1.2 ", verbatim[repeat(2)[~digit]] << double_, ut, space));
    }

    return boost::report_errors();
}
