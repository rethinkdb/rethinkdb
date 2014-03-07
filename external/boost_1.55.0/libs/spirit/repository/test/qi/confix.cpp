/*=============================================================================
    Copyright (c) 2009 Chris Hoeppler

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/qi_auxiliary.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_directive.hpp>
#include <boost/spirit/include/qi_nonterminal.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#include <boost/spirit/repository/include/qi_confix.hpp>

#include <string>
#include "test.hpp"

namespace comment {
    namespace spirit = boost::spirit;
    namespace repo = boost::spirit::repository;

    // Define a metafunction allowing to compute the type
    // of the confix() construct
    template <typename Prefix, typename Suffix = Prefix>
    struct confix_spec_traits
    {
        typedef typename spirit::result_of::terminal<
            repo::tag::confix(Prefix, Suffix)
        >::type type;
    };

    template <typename Prefix, typename Suffix>
    inline typename confix_spec_traits<Prefix, Suffix>::type
    confix_spec(Prefix const& prefix, Suffix const& suffix)
    {
        return repo::confix(prefix, suffix);
    }
    
    inline confix_spec_traits<std::string>::type
    confix_spec(const char* prefix, const char* suffix)
    {
        return repo::confix(std::string(prefix), std::string(suffix));
    }
    confix_spec_traits<std::string>::type const c_comment = confix_spec("/*", "*/");
    confix_spec_traits<std::string>::type const cpp_comment = confix_spec("//", "\n");
}

int main()
{
    using spirit_test::test_attr;
    using spirit_test::test;

    using namespace boost::spirit::ascii;
    using namespace boost::spirit::qi::labels;
    using boost::spirit::qi::locals;
    using boost::spirit::qi::rule;
    using boost::spirit::qi::debug;

    namespace phx = boost::phoenix;
    namespace repo = boost::spirit::repository;

    { // basic tests

        rule<char const*> start;

        start = repo::confix('a', 'c')['b'];
        BOOST_TEST(test("abc", start));

        start = repo::confix('a', 'c')['b'] | "abd";
        BOOST_TEST(test("abd", start));

        start = repo::confix("/*", "*/")[*(alpha - "*/")];
        BOOST_TEST(test("/*aaaabababaaabbb*/", start));

        start = repo::confix(char_('/') >> '*', '*' >> char_('/'))[*alpha - "*/"];
        BOOST_TEST(test("/*aaaabababaaabba*/", start));
        
        start = comment::c_comment[*(alpha - "*/")];
        BOOST_TEST(test("/*aaaabababaaabbb*/", start));

        // ignore the skipper!
        BOOST_TEST(!test("/* aaaabababaaabba*/", start, space));
    }

    { // basic tests w/ skipper

        rule<char const*, space_type> start;

        start = repo::confix('a', 'c')['b'];
        BOOST_TEST(test(" a b c ", start, space));

        start = repo::confix(char_('/') >> '*', '*' >> char_('/'))[*alpha - "*/"];
        BOOST_TEST(test(" / *a       b a b a b a a a b b b * / ", start, space));
    }

    { // context tests
        char ch;
        rule<char const*, char()> a;
        a = repo::confix("/*", "*/")[alpha][_val = _1];

        BOOST_TEST(test("/*x*/", a[phx::ref(ch) = _1]));
        BOOST_TEST(ch == 'x');

        a %= repo::confix("/*", "*/")[alpha];
        BOOST_TEST(test_attr("/*z*/", a, ch)); // attribute is given.
        BOOST_TEST(ch == 'z');
    }

    { // rules test
        rule<char const*> a, b, c, start;

        a = 'a';
        b = 'b';
        c = 'c';

        a.name("a");
        b.name("b");
        c.name("c");
        start.name("start");

        debug(a);
        debug(b);
        debug(c);
        debug(start);

        start = repo::confix(a.alias(), c.alias())[b];
        BOOST_TEST(test("abc", start));
    }

    { // modifiers test
        rule<char const*> start;
        start = no_case[repo::confix("_A_", "_Z_")["heLLo"]];
        BOOST_TEST(test("_a_hello_z_", start));
    }

    return boost::report_errors();
}

