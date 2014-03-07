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

    // primitive data types
    {
        utree ut('x');
        BOOST_TEST(test("x", char_, ut));

        ut = false;
        BOOST_TEST(test("false", bool_, ut));

        ut = 123;
        BOOST_TEST(test("123", int_, ut));

        ut = 123.45;
        BOOST_TEST(test("123.45", double_, ut));

        ut = "abc";
        BOOST_TEST(test("abc", string, ut));

        ut = utf8_symbol_type("xyz");
        BOOST_TEST(test("xyz", string, ut));
    }

    // sequences
    {
        using boost::spirit::karma::digit;
        using boost::spirit::karma::repeat;

        utree ut;
        ut.push_back('x');
        ut.push_back('y');
        BOOST_TEST(test("xy", char_ << char_, ut));

        ut.clear();
        ut.push_back(123);
        ut.push_back(456);
        BOOST_TEST(test_delimited("123 456 ", int_ << int_, ut, space));

        ut.clear();
        ut.push_back(1.23);
        ut.push_back(4.56);
        BOOST_TEST(test_delimited("1.23 4.56 ", double_ << double_, ut, space));

        ut.clear();
        ut.push_back(1.23);
        ut.push_back("ab");
        BOOST_TEST(test("1.23ab", double_ << string, ut));

        ut.clear();

        rule<output_iterator, double()> r1 = double_;
        rule<output_iterator, utree()> r2 = double_;

        // ( 1.23 "a" "b" )
        ut.push_back(1.23);
        ut.push_back('a');
        ut.push_back('b');
        BOOST_TEST(test("1.23ab", double_ << *char_, ut));
        BOOST_TEST(test("1.23ab", r1 << *char_, ut)); 
        BOOST_TEST(test("1.23ab", r2 << *char_, ut)); 

        // ( ( 1.23 ) "a" "b" )
        ut.clear();
        utree ut1;
        ut1.push_back(1.23);
        ut.push_back(ut1);
        ut.push_back('a');
        ut.push_back('b');
        BOOST_TEST(test("1.23ab", r1 << *char_, ut)); 
        BOOST_TEST(test("1.23ab", r2 << *char_, ut)); 

        // ( "a" "b" 1.23 )
        ut.clear();
        ut.push_back('a');
        ut.push_back('b');
        ut.push_back(1.23);
        BOOST_TEST(test("ab1.23", repeat(2)[~digit] << double_, ut));
        BOOST_TEST(test("ab1.23", repeat(2)[~digit] << r1, ut));
        BOOST_TEST(test("ab1.23", repeat(2)[~digit] << r2, ut));

        // ( "a" "b" ( 1.23 ) )
        ut.clear();
        ut.push_back('a');
        ut.push_back('b');
        ut.push_back(ut1);
        BOOST_TEST(test("ab1.23", repeat(2)[~digit] << r1, ut));
        BOOST_TEST(test("ab1.23", repeat(2)[~digit] << r2, ut));
    }

    return boost::report_errors();
}
