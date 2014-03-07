// Copyright (c) 2001-2011 Hartmut Kaiser
// Copyright (c) 2001-2011 Joel de Guzman
// Copyright (c)      2010 Bryce Lelbach
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_utree.hpp>
#include <boost/mpl/print.hpp>

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

    // as
    {
        typedef as<std::string> as_string_type;
        as_string_type const as_string = as_string_type();

        typedef as<utf8_symbol_type> as_symbol_type;
        as_symbol_type const as_symbol = as_symbol_type();

        utree ut;
        BOOST_TEST(test_attr("xy", as_string[char_ >> char_], ut) &&
            ut.which() == utree_type::string_type && check(ut, "\"xy\""));
        ut.clear();

        BOOST_TEST(test_attr("ab1.2", as_string[*~digit] >> double_, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( \"ab\" 1.2 )"));
        ut.clear();

        BOOST_TEST(test_attr("xy", as_string[*char_], ut) &&
            ut.which() == utree_type::string_type && check(ut, "\"xy\""));
        ut.clear();

        BOOST_TEST(test_attr("x,y", as_string[char_ >> ',' >> char_], ut) &&
            ut.which() == utree_type::string_type && check(ut, "\"xy\""));
        ut.clear();

        BOOST_TEST(test_attr("x,y", char_ >> ',' >> char_, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( \"x\" \"y\" )"));
        ut.clear();

        BOOST_TEST(test_attr("a,b1.2", as_string[~digit % ','] >> double_, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( \"ab\" 1.2 )"));
        ut.clear();

        BOOST_TEST(test_attr("a,b1.2", ~digit % ',' >> double_, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( \"a\" \"b\" 1.2 )"));
        ut.clear();
        
        BOOST_TEST(test_attr("xy", as_symbol[char_ >> char_], ut) &&
            ut.which() == utree_type::symbol_type && check(ut, "xy"));
        ut.clear();

        BOOST_TEST(test_attr("ab1.2", as_symbol[*~digit] >> double_, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( ab 1.2 )"));
        ut.clear();

        BOOST_TEST(test_attr("xy", as_symbol[*char_], ut) &&
            ut.which() == utree_type::symbol_type && check(ut, "xy"));
        ut.clear();

        BOOST_TEST(test_attr("x,y", as_symbol[char_ >> ',' >> char_], ut) &&
            ut.which() == utree_type::symbol_type && check(ut, "xy"));
        ut.clear();
        BOOST_TEST(test_attr("a,b1.2", as_symbol[~digit % ','] >> double_, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( ab 1.2 )"));
        ut.clear();
    }

    // subtrees
    {
        // -(+int_) is forcing a subtree
        utree ut;
        BOOST_TEST(test_attr("1 2", int_ >> ' ' >> -(+int_), ut) && 
            ut.which() == utree_type::list_type && check(ut, "( 1 2 )"));
        ut.clear();

        BOOST_TEST(test_attr("1 2", int_ >> ' ' >> *int_, ut) && 
            ut.which() == utree_type::list_type && check(ut, "( 1 2 )"));
        ut.clear();

        rule<char const*, std::vector<int>()> r1 = int_ % ',';
        BOOST_TEST(test_attr("1 2,3", int_ >> ' ' >> r1, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( 1 2 3 )"));
        ut.clear();

        BOOST_TEST(test_attr("1,2 2,3", r1 >> ' ' >> r1, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( 1 2 2 3 )")); 
        ut.clear();

        rule<char const*, utree()> r2 = int_ % ',';
        BOOST_TEST(test_attr("1 2,3", int_ >> ' ' >> r2, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( 1 2 3 )"));
        ut.clear();

        BOOST_TEST(test_attr("1,2 2,3", r2 >> ' ' >> r2, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( 1 2 2 3 )")); 
        ut.clear();

        rule<char const*, utree::list_type()> r3 = int_ % ',';
        BOOST_TEST(test_attr("1 2,3", int_ >> ' ' >> r3, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( 1 ( 2 3 ) )"));
        ut.clear();

        BOOST_TEST(test_attr("1,2 2,3", r3 >> ' ' >> r3, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( ( 1 2 ) ( 2 3 ) )"));
        ut.clear();

        rule<char const*, utree()> r4 = int_;
        BOOST_TEST(test_attr("1 1", int_ >> ' ' >> -r4, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( 1 1 )"));
        ut.clear();

        BOOST_TEST(test_attr("1 ", int_ >> ' ' >> -r4, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( 1 )"));
        ut.clear();

        rule<char const*, utree::list_type()> r5 = -r4;
        BOOST_TEST(test_attr("1", r5, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( 1 )"));
        ut.clear();

        BOOST_TEST(test_attr("", r5, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( )"));
        ut.clear();

        BOOST_TEST(test_attr("1 1", r5 >> ' ' >> r5, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( ( 1 ) ( 1 ) )"));
        ut.clear();

        rule<char const*, utree::list_type()> r6 = int_;
        rule<char const*, utree()> r7 = -r6;
        BOOST_TEST(test_attr("1", r7, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( 1 )"));
        ut.clear();

        rule<char const*, utree::list_type()> r8 = r6 >> ' ' >> r6;
        BOOST_TEST(test_attr("1 1", r8, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( ( 1 ) ( 1 ) )"));
        ut.clear();
    }

    return boost::report_errors();
}
