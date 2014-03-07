//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/fusion/include/std_pair.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <boost/spirit/include/qi_bool.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/qi_nonterminal.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_directive.hpp>
#include <boost/spirit/include/qi_auto.hpp>

#include "test.hpp"

namespace qi = boost::spirit::qi;
namespace traits = boost::spirit::traits;

///////////////////////////////////////////////////////////////////////////////
template <typename Char, typename T>
bool test_create_parser(Char const *in, T& t)
{
    Char const* last = in;
    while (*last)
        last++;

    BOOST_TEST((traits::meta_create_exists<qi::domain, T>::value));
    return qi::phrase_parse(in, last, qi::create_parser<T>(), qi::space, t);
}

template <typename Char, typename T>
bool test_create_parser_auto(Char const *in, T& t)
{
    Char const* last = in;
    while (*last)
        last++;

    BOOST_TEST((traits::meta_create_exists<qi::domain, T>::value));
    return qi::phrase_parse(in, last, t, qi::space);
}

template <typename Char, typename Attribute>
bool test_rule(Char const* in, Attribute const& expected)
{
    BOOST_TEST((traits::meta_create_exists<qi::domain, Attribute>::value));

    Attribute attr = Attribute();
    qi::rule<Char const*, Attribute()> r = qi::create_parser<Attribute>();
    return spirit_test::test_attr(in, r, attr) && attr == expected;
}

template <typename Char, typename Attribute, typename Skipper>
bool test_rule(Char const* in, Attribute const& expected, Skipper const& skipper)
{
    BOOST_TEST((traits::meta_create_exists<qi::domain, Attribute>::value));

    Attribute attr = Attribute();
    qi::rule<Char const*, Attribute(), Skipper> r = qi::create_parser<Attribute>();
    return spirit_test::test_attr(in, r, attr, skipper) && attr == expected;
}

struct my_type {};

///////////////////////////////////////////////////////////////////////////////
int main()
{
    {
        BOOST_TEST((!traits::meta_create_exists<qi::domain, my_type>::value));
    }

    {
        // test primitive types
        bool b = false;
        BOOST_TEST(test_create_parser("true", b) && b == true);
        int i = 0;
        BOOST_TEST(test_create_parser("1", i) && i == 1);
        double d = 0;
        BOOST_TEST(test_create_parser("1.1", d) && d == 1.1);
        char c = '\0';
        BOOST_TEST(test_create_parser("a", c) && c == 'a');
        wchar_t wc = L'\0';
        BOOST_TEST(test_create_parser(L"a", wc) && wc == L'a');

        // test containers
        std::vector<int> v;
        BOOST_TEST(test_create_parser("0 1 2", v) && v.size() == 3 && 
            v[0] == 0 && v[1] == 1 && v[2] == 2);

        std::list<int> l;
        BOOST_TEST(test_create_parser("0 1 2", l) && l.size() == 3 &&
            *l.begin() == 0 && *(++l.begin()) == 1 && *(++ ++l.begin()) == 2);

        // test optional
        boost::optional<int> o;
        BOOST_TEST(test_create_parser("", o) && !o);
        BOOST_TEST(test_create_parser("1", o) && !!o && boost::get<int>(o) == 1);

        // test alternative
        boost::variant<double, bool, std::vector<char> > vv;
        BOOST_TEST(test_create_parser("true", vv) && vv.which() == 1 &&
            boost::get<bool>(vv) == true);
        BOOST_TEST(test_create_parser("1.0", vv) && vv.which() == 0 &&
            boost::get<double>(vv) == 1.0);
        BOOST_TEST(test_create_parser("some_string", vv) && vv.which() == 2 &&
            boost::equals(boost::get<std::vector<char> >(vv), "some_string"));

        // test fusion sequence
        std::pair<int, double> p;
        BOOST_TEST(test_create_parser("1 2.0", p) && 
            p.first == 1 && p.second == 2.0);
    }

    {
        // test containers
        std::vector<int> v;
        BOOST_TEST(test_create_parser_auto("0 1 2", v) && v.size() == 3 && 
            v[0] == 0 && v[1] == 1 && v[2] == 2);

        std::list<int> l;
        BOOST_TEST(test_create_parser_auto("0 1 2", l) && l.size() == 3 &&
            *l.begin() == 0 && *(++l.begin()) == 1 && *(++ ++l.begin()) == 2);

        // test optional
        boost::optional<int> o;
        BOOST_TEST(test_create_parser_auto("", o) && !o);
        BOOST_TEST(test_create_parser_auto("1", o) && !!o && boost::get<int>(o) == 1);

        // test alternative
        boost::variant<double, bool, std::vector<char> > vv;
        BOOST_TEST(test_create_parser_auto("true", vv) && vv.which() == 1 &&
            boost::get<bool>(vv) == true);
        BOOST_TEST(test_create_parser_auto("1.0", vv) && vv.which() == 0 &&
            boost::get<double>(vv) == 1.0);
        BOOST_TEST(test_create_parser_auto("some_string", vv) && vv.which() == 2 &&
            boost::equals(boost::get<std::vector<char> >(vv), "some_string"));

        // test fusion sequence
        std::pair<int, double> p;
        BOOST_TEST(test_create_parser_auto("1 2.0", p) && 
            p.first == 1 && p.second == 2.0);
    }

    {
        using qi::auto_;
        using qi::no_case;
        using spirit_test::test_attr;

        // test primitive types
        bool b = false;
        BOOST_TEST(test_attr("true", auto_, b) && b == true);
        int i = 0;
        BOOST_TEST(test_attr("1", auto_, i) && i == 1);
        double d = 0;
        BOOST_TEST(test_attr("1.1", auto_, d) && d == 1.1);
        char c = '\0';
        BOOST_TEST(test_attr("a", auto_, c) && c == 'a');
        wchar_t wc = L'\0';
        BOOST_TEST(test_attr(L"a", auto_, wc) && wc == L'a');

        b = false;
        BOOST_TEST(test_attr("TRUE", no_case[auto_], b) && b == true);

        // test containers
        std::vector<int> v;
        BOOST_TEST(test_attr("0 1 2", auto_, v, qi::space) && v.size() == 3 && 
            v[0] == 0 && v[1] == 1 && v[2] == 2);
        v.clear();
        BOOST_TEST(test_attr("0,1,2", auto_ % ',', v) && v.size() == 3 && 
            v[0] == 0 && v[1] == 1 && v[2] == 2);

        std::list<int> l;
        BOOST_TEST(test_attr("0 1 2", auto_, l, qi::space) && l.size() == 3 &&
            *l.begin() == 0 && *(++l.begin()) == 1 && *(++ ++l.begin()) == 2);
        l.clear();
        BOOST_TEST(test_attr("0,1,2", auto_ % ',', l) && l.size() == 3 &&
            *l.begin() == 0 && *(++l.begin()) == 1 && *(++ ++l.begin()) == 2);

        // test optional
        boost::optional<int> o;
        BOOST_TEST(test_attr("", auto_, o) && !o);
        BOOST_TEST(test_attr("1", auto_, o) && !!o && boost::get<int>(o) == 1);

        // test alternative
        boost::variant<double, bool, std::vector<char> > vv;
        BOOST_TEST(test_attr("true", auto_, vv) && vv.which() == 1 &&
            boost::get<bool>(vv) == true);
        BOOST_TEST(test_attr("1.0", auto_, vv) && vv.which() == 0 &&
            boost::get<double>(vv) == 1.0);
        BOOST_TEST(test_create_parser("some_string", vv) && vv.which() == 2 &&
            boost::equals(boost::get<std::vector<char> >(vv), "some_string"));

        // test fusion sequence
        std::pair<int, double> p;
        BOOST_TEST(test_attr("1 2.0", auto_, p, qi::space) && 
            p.first == 1 && p.second == 2.0);
    }

    {
        // test primitive types
        BOOST_TEST(test_rule("true", true));
        BOOST_TEST(test_rule("1", 1));
        BOOST_TEST(test_rule("1.1", 1.1));

        // test containers
        std::vector<int> v;
        v.push_back(0);
        v.push_back(1);
        v.push_back(2);
        BOOST_TEST(test_rule("0 1 2", v, qi::space));

        std::list<int> l;
        l.push_back(0);
        l.push_back(1);
        l.push_back(2);
        BOOST_TEST(test_rule("0 1 2", l, qi::space));

        // test optional
        boost::optional<int> o;
        BOOST_TEST(test_rule("", o));
        o = 1;
        BOOST_TEST(test_rule("1", o));

        // test alternative
//         boost::variant<int, double, float, std::string> vv;
//         vv = 1;
//         BOOST_TEST(test_rule("1", vv));
//         vv = 1.0;
//         BOOST_TEST(test_rule("1.0", vv));
//         vv = 1.0f;
//         BOOST_TEST(test_rule("1.0", vv));
//         vv = "some string";
//         BOOST_TEST(test_rule("some string", vv));

        // test fusion sequence
        std::pair<int, double> p (1, 2.0);
        BOOST_TEST(test_rule("1 2.0", p, qi::space));
    }

    return boost::report_errors();
}
