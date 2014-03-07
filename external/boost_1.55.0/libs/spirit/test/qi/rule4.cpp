/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// this file deliberately contains non-ascii characters
// boostinspect:noascii

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_auxiliary.hpp>
#include <boost/spirit/include/qi_directive.hpp>
#include <boost/spirit/include/qi_nonterminal.hpp>
#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>
#include <boost/fusion/include/std_pair.hpp>

#include <string>
#include <cstring>
#include <iostream>
#include "test.hpp"

int
main()
{
    using spirit_test::test_attr;
    using spirit_test::test;

    using namespace boost::spirit::ascii;
    using namespace boost::spirit::qi::labels;
    using boost::spirit::qi::locals;
    using boost::spirit::qi::rule;
    using boost::spirit::qi::int_;
    using boost::spirit::qi::uint_;
    using boost::spirit::qi::fail;
    using boost::spirit::qi::on_error;
    using boost::spirit::qi::debug;
    using boost::spirit::qi::lit;

    namespace phx = boost::phoenix;


    { // show that ra = rb and ra %= rb works as expected
        rule<char const*, int() > ra, rb;
        int attr;

        ra %= int_;
        BOOST_TEST(test_attr("123", ra, attr));
        BOOST_TEST(attr == 123);

        rb %= ra;
        BOOST_TEST(test_attr("123", rb, attr));
        BOOST_TEST(attr == 123);

        rb = ra;
        BOOST_TEST(test_attr("123", rb, attr));
        BOOST_TEST(attr == 123);
    }

    { // std::string as container attribute with auto rules

        rule<char const*, std::string()> text;
        text %= +(!char_(')') >> !char_('>') >> char_);
        std::string attr;
        BOOST_TEST(test_attr("x", text, attr));
        BOOST_TEST(attr == "x");

        // test deduced auto rule behavior
        text = +(!char_(')') >> !char_('>') >> char_);
        attr.clear();
        BOOST_TEST(test_attr("x", text, attr));
        BOOST_TEST(attr == "x");
    }

    { // error handling

        using namespace boost::spirit::ascii;
        using boost::phoenix::construct;
        using boost::phoenix::bind;

        rule<char const*> r;
        r = '(' > int_ > ',' > int_ > ')';

        on_error<fail>
        (
            r, std::cout
                << phx::val("Error! Expecting: ")
                << _4
                << phx::val(", got: \"")
                << construct<std::string>(_3, _2)
                << phx::val("\"")
                << std::endl
        );

        BOOST_TEST(test("(123,456)", r));
        BOOST_TEST(!test("(abc,def)", r));
        BOOST_TEST(!test("(123,456]", r));
        BOOST_TEST(!test("(123;456)", r));
        BOOST_TEST(!test("[123,456]", r));
    }

#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1310))
#pragma setlocale("french")
#endif
    { // specifying the encoding

        typedef boost::spirit::char_encoding::iso8859_1 iso8859_1;
        rule<char const*, iso8859_1> r;

        r = no_case['·'];
        BOOST_TEST(test("¡", r));
        r = no_case[char_('·')];
        BOOST_TEST(test("¡", r));

        r = no_case[char_("Â-Ô")];
        BOOST_TEST(test("…", r));
        BOOST_TEST(!test("ˇ", r));

        r = no_case["·¡"];
        BOOST_TEST(test("¡·", r));
        r = no_case[lit("·¡")];
        BOOST_TEST(test("¡·", r));
    }

#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1310))
#pragma setlocale("")
#endif

    {
        typedef boost::variant<double, int> v_type;
        rule<const char*, v_type()> r1 = int_;
        v_type v;
        BOOST_TEST(test_attr("1", r1, v) && v.which() == 1 &&
            boost::get<int>(v) == 1);

        typedef boost::optional<int> ov_type;
        rule<const char*, ov_type()> r2 = int_;
        ov_type ov;
        BOOST_TEST(test_attr("1", r2, ov) && ov && boost::get<int>(ov) == 1);
    }

    // test handling of single element fusion sequences
    {
        using boost::fusion::vector;
        using boost::fusion::at_c;
        rule<const char*, vector<int>()> r = int_;

        vector<int> v(0);
        BOOST_TEST(test_attr("1", r, v) && at_c<0>(v) == 1);
    }

    {
        using boost::fusion::vector;
        using boost::fusion::at_c;
        rule<const char*, vector<unsigned int>()> r = uint_;

        vector<unsigned int> v(0);
        BOOST_TEST(test_attr("1", r, v) && at_c<0>(v) == 1);
    }

    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::qi::int_;
        using boost::spirit::qi::_1;
        using boost::spirit::qi::_val;
        using boost::spirit::qi::space;
        using boost::spirit::qi::space_type;

        rule<const char*, int()> r1 = int_;
        rule<const char*, int(), space_type> r2 = int_;

        int i = 0;
        int j = 0;
        BOOST_TEST(test_attr("456", r1[_val = _1], i) && i == 456);
        BOOST_TEST(test_attr("   456", r2[_val = _1], j, space) && j == 456);
    }

#if 0 // disabling test (can't fix)
    {
        using boost::spirit::qi::lexeme;
        using boost::spirit::qi::alnum;

        rule<const char*, std::string()> literal_;
        literal_ = lexeme[ +(alnum | '_') ];

        std::string attr;
        BOOST_TEST(test_attr("foo_bar", literal_, attr) && attr == "foo_bar");
        std::cout << attr << std::endl;
    }
#endif

    return boost::report_errors();
}

