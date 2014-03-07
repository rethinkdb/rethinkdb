/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/mpl/print.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_directive.hpp>
#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/qi_nonterminal.hpp>
#include <boost/spirit/include/qi_auxiliary.hpp>
#include <boost/spirit/include/qi_rule.hpp>
#include <boost/spirit/include/support_argument.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant.hpp>
#include <boost/assert.hpp>

#include <string>
#include <iostream>
#include <vector>
#include "test.hpp"

struct test_action
{
    test_action(char last)
      : last_(last) {}

    void operator()(std::vector<char> const& v
      , boost::spirit::unused_type
      , boost::spirit::unused_type) const
    {
        BOOST_TEST(v.size() == 4 &&
            v[0] == 'a' && v[1] == 'b' && v[2] == '1' && v[3] == last_);
    }

    char last_;
};

struct test_action_2
{
    typedef std::vector<boost::optional<char> > result_type;

    void operator()(result_type const& v
      , boost::spirit::unused_type
      , boost::spirit::unused_type) const
    {
        BOOST_TEST(v.size() == 5 &&
            !v[0] && v[1] == 'a' && v[2] == 'b' && v[3] == '1' && v[4] == '2');
    }
};

struct DIgnore
{
    std::string text;
};

struct DInclude
{
    std::string FileName;
};

BOOST_FUSION_ADAPT_STRUCT(
    DIgnore,
    (std::string, text)
)

BOOST_FUSION_ADAPT_STRUCT(
    DInclude,
    (std::string, FileName)
)

int
main()
{
    using spirit_test::test;
    using spirit_test::test_attr;

    using boost::spirit::ascii::char_;
    using boost::spirit::qi::int_;
    using boost::spirit::qi::lit;
    using boost::spirit::qi::unused_type;
    using boost::spirit::qi::_1;
    using boost::spirit::qi::omit;

    {
        BOOST_TEST((test("a", char_ | char_)));
        BOOST_TEST((test("x", lit('x') | lit('i'))));
        BOOST_TEST((test("i", lit('x') | lit('i'))));
        BOOST_TEST((!test("z", lit('x') | lit('o'))));
        BOOST_TEST((test("rock", lit("rock") | lit("roll"))));
        BOOST_TEST((test("roll", lit("rock") | lit("roll"))));
        BOOST_TEST((test("rock", lit("rock") | int_)));
        BOOST_TEST((test("12345", lit("rock") | int_)));
    }

    {
        boost::variant<unused_type, int, char> v;

        BOOST_TEST((test_attr("12345", lit("rock") | int_ | char_, v)));
        BOOST_TEST(boost::get<int>(v) == 12345);

        BOOST_TEST((test_attr("rock", lit("rock") | int_ | char_, v)));
        BOOST_TEST(v.which() == 1);

        BOOST_TEST((test_attr("x", lit("rock") | int_ | char_, v)));
        BOOST_TEST(boost::get<char>(v) == 'x');
    }

    {   // Make sure that we are using the actual supplied attribute types
        // from the variant and not the expected type.
        // $$$ Fixed: <2/19/2011> JDG $$$
         boost::variant<int, std::string> v;
         BOOST_TEST((test_attr("12345", int_ | +char_, v)));
         BOOST_TEST(boost::get<int>(v) == 12345);

         BOOST_TEST((test_attr("abc", int_ | +char_, v)));
         BOOST_TEST(boost::get<std::string>(v) == "abc");

         BOOST_TEST((test_attr("12345", +char_ | int_, v)));
         BOOST_TEST(boost::get<std::string>(v) == "12345");
    }

    {   // test action

        namespace phx = boost::phoenix;
        boost::optional<boost::variant<int, char> > v;

        BOOST_TEST((test("12345", (lit("rock") | int_ | char_)[phx::ref(v) = _1])));
        BOOST_TEST(boost::get<int>(boost::get(v)) == 12345);
        BOOST_TEST((test("rock", (lit("rock") | int_ | char_)[phx::ref(v) = _1])));
        BOOST_TEST(!v);
    }

    {
        unused_type x;
        BOOST_TEST((test_attr("rock", lit("rock") | lit('x'), x)));
    }

    {
        // test if alternatives with all components having unused
        // attributes have an unused attribute

        using boost::fusion::vector;
        using boost::fusion::at_c;

        vector<char, char> v;
        BOOST_TEST((test_attr("abc",
            char_ >> (omit[char_] | omit[char_]) >> char_, v)));
        BOOST_TEST((at_c<0>(v) == 'a'));
        BOOST_TEST((at_c<1>(v) == 'c'));
    }

    {
        // Test that we can still pass a "compatible" attribute to
        // an alternate even if its "expected" attribute is unused type.

        std::string s;
        BOOST_TEST((test_attr("...", *(char_('.') | char_(',')), s)));
        BOOST_TEST(s == "...");
    }

    {   // make sure collapsing eps works as expected
        // (compile check only)

        using boost::spirit::qi::rule;
        using boost::spirit::qi::_val;
        using boost::spirit::qi::_1;
        using boost::spirit::eps;
        rule<const wchar_t*, wchar_t()> r1, r2, r3;

        r3  = ((eps >> r1))[_val += _1];
        r3  = ((r1 ) | r2)[_val += _1];

        r3 %= ((eps >> r1) | r2);
        r3 = ((eps >> r1) | r2)[_val += _1];
    }

    // make sure the attribute of an alternative gets properly collapsed
    {
        using boost::spirit::qi::lexeme;
        using boost::spirit::ascii::alnum;
        using boost::spirit::ascii::alpha;
        using boost::spirit::ascii::digit;
        using boost::spirit::ascii::string;
        namespace phx = boost::phoenix;


        BOOST_TEST( (test("ab1_", (*(alnum | char_('_')))[test_action('_')])) );
        BOOST_TEST( (test("ab12", (*(alpha | digit))[test_action('2')])) );

        BOOST_TEST( (test("abcab12", (*("abc" | alnum))[test_action_2()])) );

        std::vector<boost::optional<char> > v;
        BOOST_TEST( (test("x,y,z", (*(',' | char_))[phx::ref(v) = _1])) );
        BOOST_ASSERT(v[0] == 'x');
        BOOST_ASSERT(!v[1]);
        BOOST_ASSERT(v[2] == 'y');
        BOOST_ASSERT(!v[3]);
        BOOST_ASSERT(v[4] == 'z');
    }

    {
        using boost::spirit::qi::eps;

        // testing a sequence taking a container as attribute
        std::string s;
        BOOST_TEST( (test_attr("abc,a,b,c",
            char_ >> char_ >> (char_ % ','), s )) );
        BOOST_TEST(s == "abcabc");

        // test having an optional<container> inside a sequence
        s.erase();
        BOOST_TEST( (test_attr("ab",
            char_ >> char_ >> -(char_ % ','), s )) );
        BOOST_TEST(s == "ab");

        // test having a variant<container, ...> inside a sequence
        s.erase();
        BOOST_TEST( (test_attr("ab",
            char_ >> char_ >> ((char_ % ',') | eps), s )) );
        BOOST_TEST(s == "ab");
        s.erase();
        BOOST_TEST( (test_attr("abc",
            char_ >> char_ >> ((char_ % ',') | eps), s )) );
        BOOST_TEST(s == "abc");
    }

    {
        using boost::spirit::qi::int_;

        int i = 0;
        BOOST_TEST( (test_attr("10", int_(5) | int_(10), i)) );
        BOOST_TEST(i == 10);
    }

    {
        //compile test only (bug_march_10_2011_8_35_am)
        typedef boost::variant<double, std::string> value_type;

        using boost::spirit::qi::rule;
        using boost::spirit::qi::eps;
        rule<std::string::const_iterator, value_type()> r1 = r1 | eps;
    }

    {
        using boost::spirit::qi::rule;
        typedef boost::variant<DIgnore, DInclude> DLine;

        rule<char*, DIgnore()> ignore;
        rule<char*, DInclude()> include;
        rule<char*, DLine()> line = include | ignore;
    }

    return boost::report_errors();
}

