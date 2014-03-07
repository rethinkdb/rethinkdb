/*=============================================================================
    Copyright (c) 2004 Joao Abecasis
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <boost/spirit/include/classic_parser.hpp>
#include <boost/spirit/include/classic_skipper.hpp>
#include <boost/spirit/include/classic_primitives.hpp>
#include <boost/spirit/include/classic_optional.hpp>
#include <boost/spirit/include/classic_sequence.hpp>
#include <boost/spirit/include/classic_ast.hpp>
#include <boost/spirit/include/classic_parse_tree.hpp>

#include <boost/detail/lightweight_test.hpp>

using namespace BOOST_SPIRIT_CLASSIC_NS;

char const * test1 = "  12345  ";
char const * test2 = "  12345  x";

void parse_tests()
{
    parse_info<> info;

    // Warming up...
    info = parse(test1, str_p("12345"));
    BOOST_TEST(!info.hit);

    // No post-skips!
    info = parse(test1, str_p("12345"), blank_p);
    BOOST_TEST(info.hit);
    BOOST_TEST(!info.full);

    // Require a full match
    info = parse(test1, str_p("12345") >> end_p, blank_p);
    BOOST_TEST(info.full);
    info = parse(test2, str_p("12345") >> end_p, blank_p);
    BOOST_TEST(!info.hit);

    // Check for a full match but don't make it a requirement
    info = parse(test1, str_p("12345") >> !end_p, blank_p);
    BOOST_TEST(info.full);
    info = parse(test2, str_p("12345") >> !end_p, blank_p);
    BOOST_TEST(info.hit);
    BOOST_TEST(!info.full);
}

void ast_parse_tests()
{
    tree_parse_info<> info;

    // Warming up...
    info = ast_parse(test1, str_p("12345"));
    BOOST_TEST(!info.match);

    // No post-skips!
    info = ast_parse(test1, str_p("12345"), blank_p);
    BOOST_TEST(info.match);
    BOOST_TEST(!info.full);

    // Require a full match
    info = ast_parse(test1, str_p("12345") >> end_p, blank_p);
    BOOST_TEST(info.full);
    info = ast_parse(test2, str_p("12345") >> end_p, blank_p);
    BOOST_TEST(!info.match);

    // Check for a full match but don't make it a requirement
    info = ast_parse(test1, str_p("12345") >> !end_p, blank_p);
    BOOST_TEST(info.full);
    info = ast_parse(test2, str_p("12345") >> !end_p, blank_p);
    BOOST_TEST(info.match);
    BOOST_TEST(!info.full);
}

void pt_parse_tests()
{
    tree_parse_info<> info;

    // Warming up...
    info = pt_parse(test1, str_p("12345"));
    BOOST_TEST(!info.match);

    // No post-skips!
    info = pt_parse(test1, str_p("12345"), blank_p);
    BOOST_TEST(info.match);
    BOOST_TEST(!info.full);

    // Require a full match
    info = pt_parse(test1, str_p("12345") >> end_p, blank_p);
    BOOST_TEST(info.full);
    info = pt_parse(test2, str_p("12345") >> end_p, blank_p);
    BOOST_TEST(!info.match);

    // Check for a full match but don't make it a requirement
    info = pt_parse(test1, str_p("12345") >> !end_p, blank_p);
    BOOST_TEST(info.full);
    info = pt_parse(test2, str_p("12345") >> !end_p, blank_p);
    BOOST_TEST(info.match);
    BOOST_TEST(!info.full);
}

int main()
{
    parse_tests();
    ast_parse_tests();
    pt_parse_tests();

    return boost::report_errors();
}
