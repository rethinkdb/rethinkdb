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

    // kleene star
    {
        utree ut;
        ut.push_back('a');
        ut.push_back('b');
        BOOST_TEST(test("ab", *char_, ut));

        ut.clear();
        ut.push_back(123);
        ut.push_back(456);
        BOOST_TEST(test_delimited("123 456 ", *int_, ut, space));

        ut.clear();
        ut.push_back(1.23);
        ut.push_back(4.56);
        BOOST_TEST(test_delimited("1.23 4.56 ", *double_, ut, space));
    }

    // lists
    {
        rule<output_iterator, utree()> r1, r1ref;
        rule<output_iterator, utf8_string_range_type()> r1str;
        rule<output_iterator, utree::const_range()> r1list;

        r1 = double_ | int_ | r1str | r1list | r1ref;

        r1ref = r1.alias();

        r1str = string;

        r1list = '(' << -(r1 % ',') << ')';

        // ( "abc" "def" )
        utree ut;
        ut.push_back("abc");
        ut.push_back("def");
        BOOST_TEST(test("abc,def", string % ',', ut));
        BOOST_TEST(test("(abc,def)", r1, ut));

        // ( ( "abc" "def" ) )
        utree ut1;
        ut1.push_back(ut);
        BOOST_TEST(test("((abc,def))", r1, ut1));

//         rule<output_iterator, std::vector<char>()> r2 = char_ % ',';
//         BOOST_TEST(test("abc,def", r2, ut));
//         BOOST_TEST(test("abc,def", r2, ut1));

        // ( ( "abc" "def" ) ( "abc" "def" ) )
        ut1.push_back(ut);
        BOOST_TEST(test("(abc,def) (abc,def)", r1 << ' ' << r1, ut1));

        // ( 123 456 )
        ut.clear();
        ut.push_back(123);
        ut.push_back(456);
        BOOST_TEST(test("123,456", int_ % ',', ut));
        BOOST_TEST(test("(123,456)", r1, ut));

        // ( ( 123 456 ) ) 
        ut1.clear();
        ut1.push_back(ut);
        BOOST_TEST(test("((123,456))", r1, ut1));

//         rule<output_iterator, std::vector<int>()> r4 = int_ % ',';
//         BOOST_TEST(test("123,456", r4, ut));
//         BOOST_TEST(test("123,456", r4, ut1));

        // ( ( 123 456 ) ( 123 456 ) ) 
        ut1.push_back(ut);
        BOOST_TEST(test("(123,456) (123,456)", r1 << ' ' << r1, ut1));

        // ( 1.23 4.56 ) 
        ut.clear();
        ut.push_back(1.23);
        ut.push_back(4.56);
        BOOST_TEST(test("1.23,4.56", double_ % ',', ut));
        BOOST_TEST(test("(1.23,4.56)", r1, ut));

        // ( ( 1.23 4.56 ) )
        ut1.clear();
        ut1.push_back(ut);
        BOOST_TEST(test("((1.23,4.56))", r1, ut1));

//         rule<output_iterator, std::vector<double>()> r6 = double_ % ',';
//         BOOST_TEST(test("1.23,4.56", r6, ut));
//         BOOST_TEST(test("1.23,4.56", r6, ut1));

        // ( ( 1.23 4.56 ) ( 1.23 4.56 ) )
        ut1.push_back(ut);
        BOOST_TEST(test("(1.23,4.56) (1.23,4.56)", r1 <<' ' << r1, ut1));
    }

    // alternatives
    {
        rule<output_iterator, utree()> r1 = int_ | double_;
        utree ut(10);
        BOOST_TEST(test("10", int_ | double_, ut));
        BOOST_TEST(test("10", r1, ut));

        ut = 10.2;
        BOOST_TEST(test("10.2", int_ | double_, ut));
        BOOST_TEST(test("10.2", r1, ut));
    }

    // optionals
    {
        utree ut('x');
        BOOST_TEST(test("x", -char_, ut));

        ut.clear();
        BOOST_TEST(test("", -char_, ut));
    }

    return boost::report_errors();
}
