// Copyright (c) 2001-2011 Hartmut Kaiser
// Copyright (c) 2001-2011 Joel de Guzman
// Copyright (c)      2010 Bryce Lelbach
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/mpl/print.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_utree.hpp>

#include <sstream>

#include "test.hpp"

inline bool check(boost::spirit::utree const& val, std::string expected)
{
    std::stringstream s;
    s << val;
    if (s.str() == expected + " ")
        return true;

    std::cerr << "got result: " << s.str() 
              << ", expected: " << expected << std::endl;
    return false;
}

int main()
{
    using spirit_test::test_attr;
    using boost::spirit::utree;
    using boost::spirit::utree_type;
    using boost::spirit::utf8_string_range_type;
    using boost::spirit::utf8_symbol_type;
    using boost::spirit::utf8_string_type;

    using boost::spirit::qi::real_parser;
    using boost::spirit::qi::strict_real_policies;
    using boost::spirit::qi::digit;
    using boost::spirit::qi::char_;
    using boost::spirit::qi::string;
    using boost::spirit::qi::int_;
    using boost::spirit::qi::double_;
    using boost::spirit::qi::space;
    using boost::spirit::qi::space_type;
    using boost::spirit::qi::rule;
    using boost::spirit::qi::as;
    using boost::spirit::qi::lexeme;

    // primitive data types
    {
        utree ut;
        BOOST_TEST(test_attr("x", char_, ut) &&
            ut.which() == utree_type::string_type && check(ut, "\"x\""));
        ut.clear();
        BOOST_TEST(test_attr("123", int_, ut) &&
            ut.which() == utree_type::int_type && check(ut, "123"));
        ut.clear();
        BOOST_TEST(test_attr("123.45", double_, ut) &&
            ut.which() == utree_type::double_type && check(ut, "123.45"));
        ut.clear();

        rule<char const*, utf8_string_type()> r1 = lexeme[*char_];

        BOOST_TEST(test_attr("foo", r1, ut) &&
            ut.which() == utree_type::string_type && check(ut, "\"foo\""));
        ut.clear();
        
        rule<char const*, utf8_symbol_type()> r2 = lexeme[*char_];
        
        BOOST_TEST(test_attr("xyz", r2, ut) &&
            ut.which() == utree_type::symbol_type && check(ut, "xyz"));
    }

    // single character parsers
    {
        utree ut;

        // this rule returns a utree string
        rule<char const*, utree()> r1 = char_("abc");

        // this rule forces a utree list to be returned
        rule<char const*, utree::list_type()> r2 = char_("abc");

        BOOST_TEST(test_attr("a", r1, ut) &&
            ut.which() == utree_type::string_type && check(ut, "\"a\""));
        ut.clear();
        BOOST_TEST(test_attr("a", r2, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( \"a\" )"));
    }

    // sequences
    {
        using boost::spirit::qi::as_string;

        utree ut;
        BOOST_TEST(test_attr("xy", char_ >> char_, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( \"x\" \"y\" )"));
        ut.clear();
        BOOST_TEST(test_attr("123 456", int_ >> int_, ut, space) &&
            ut.which() == utree_type::list_type && check(ut, "( 123 456 )"));
        ut.clear();
        BOOST_TEST(test_attr("1.23 4.56", double_ >> double_, ut, space) &&
            ut.which() == utree_type::list_type && check(ut, "( 1.23 4.56 )"));
        ut.clear();
        BOOST_TEST(test_attr("1.2ab", double_ >> *char_, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( 1.2 \"a\" \"b\" )"));
        ut.clear();
        BOOST_TEST(test_attr("ab1.2", *~digit >> double_, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( \"a\" \"b\" 1.2 )"));

        // forces a utree list
        rule<char const*, utree::list_type()> r1 = double_;

        ut.clear();
        BOOST_TEST(test_attr("1.2ab", r1 >> *char_, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( ( 1.2 ) \"a\" \"b\" )")); 
        ut.clear();
        BOOST_TEST(test_attr("ab1.2", *~digit >> r1, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( \"a\" \"b\" ( 1.2 ) )")); 
        ut.clear();
       
        // implicitly a utree list, because of sequence attribute rules 
        rule<char const*, utree()> r2 = int_ >> char_("!") >> double_;
        
        BOOST_TEST(test_attr("17!3.14", r2, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( 17 \"!\" 3.14 )"));
        ut.clear();
        
        rule<char const*, utree()> r3 = double_ >> as_string[string("foo")] >> int_;

        BOOST_TEST(test_attr("0.5foo5", r3, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( 0.5 \"foo\" 5 )"));
    }

    {
        utree ut;
 
        rule<char const*, utree()> r1 = char_;
        rule<char const*, utree::list_type()> r2 = double_;
        rule<char const*, utree::list_type()> r3 = char_;

        BOOST_TEST(test_attr("a25.5b", r1 >> r2 >> r3, ut));
        BOOST_TEST(ut.which() == utree_type::list_type);
        BOOST_TEST(check(ut, "( \"a\" ( 25.5 ) ( \"b\" ) )"));
        ut.clear();

        BOOST_TEST(test_attr("a25.5b", r3 >> r2 >> r1, ut));
        BOOST_TEST(ut.which() == utree_type::list_type);
        BOOST_TEST(check(ut, "( ( \"a\" ) ( 25.5 ) \"b\" )"));
        ut.clear();

        BOOST_TEST(test_attr("a25.5b", char_ >> r2 >> r3, ut));
        BOOST_TEST(ut.which() == utree_type::list_type);
        BOOST_TEST(check(ut, "( \"a\" ( 25.5 ) ( \"b\" ) )"));
        ut.clear();

        BOOST_TEST(test_attr("a25.5b", r3 >> r2 >> char_, ut));
        BOOST_TEST(ut.which() == utree_type::list_type);
        BOOST_TEST(check(ut, "( ( \"a\" ) ( 25.5 ) \"b\" )"));
        ut.clear();

        BOOST_TEST(test_attr("a25.5b", r1 > r2 >> r3, ut));
        BOOST_TEST(ut.which() == utree_type::list_type);
        BOOST_TEST(check(ut, "( \"a\" ( 25.5 ) ( \"b\" ) )"));
        ut.clear();

        BOOST_TEST(test_attr("a25.5b", r3 >> r2 > r1, ut));
        BOOST_TEST(ut.which() == utree_type::list_type);
        BOOST_TEST(check(ut, "( ( \"a\" ) ( 25.5 ) \"b\" )"));
        ut.clear();

        BOOST_TEST(test_attr("a25.5b", char_ > r2 >> r3, ut));
        BOOST_TEST(ut.which() == utree_type::list_type);
        BOOST_TEST(check(ut, "( \"a\" ( 25.5 ) ( \"b\" ) )"));
        ut.clear();

        BOOST_TEST(test_attr("a25.5b", r3 >> r2 > char_, ut));
        BOOST_TEST(ut.which() == utree_type::list_type);
        BOOST_TEST(check(ut, "( ( \"a\" ) ( 25.5 ) \"b\" )"));
    }

    return boost::report_errors();
}

