/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/qi_auxiliary.hpp>
#include <boost/spirit/include/qi_directive.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_nonterminal.hpp>
#include <boost/spirit/include/support_argument.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#include <iostream>
#include "test.hpp"

// Custom string type with a C-style string conversion.
struct custom_string_c
{
    custom_string_c(char c) { str[0] = c; str[1] = '\0'; }

    operator char*() { return str; }
    operator char const*() const { return str; }

private:
    char str[2];
};

std::string get_str(char const* str)
{
    return std::string(str);
}

int
main()
{
    using spirit_test::test;
    using spirit_test::test_attr;
    using boost::spirit::qi::symbols;
    using boost::spirit::qi::rule;
    using boost::spirit::qi::lazy;
    using boost::spirit::qi::_r1;

    { // construction from symbol array
        char const* syms[] = {"Joel","Ruby","Tenji","Tutit","Kim","Joey"};
        symbols<char, int> sym(syms);

        BOOST_TEST((test("Joel", sym)));
        BOOST_TEST((test("Ruby", sym)));
        BOOST_TEST((test("Tenji", sym)));
        BOOST_TEST((test("Tutit", sym)));
        BOOST_TEST((test("Kim", sym)));
        BOOST_TEST((test("Joey", sym)));
        BOOST_TEST((!test("XXX", sym)));
    }

    { // construction from 2 arrays

        char const* syms[] = {"Joel","Ruby","Tenji","Tutit","Kim","Joey"};
        int data[] = {1,2,3,4,5,6};
        symbols<char, int> sym(syms, data);

        int i;
        BOOST_TEST((test_attr("Joel", sym, i)));
        BOOST_TEST(i == 1);
        BOOST_TEST((test_attr("Ruby", sym, i)));
        BOOST_TEST(i == 2);
        BOOST_TEST((test_attr("Tenji", sym, i)));
        BOOST_TEST(i == 3);
        BOOST_TEST((test_attr("Tutit", sym, i)));
        BOOST_TEST(i == 4);
        BOOST_TEST((test_attr("Kim", sym, i)));
        BOOST_TEST(i == 5);
        BOOST_TEST((test_attr("Joey", sym, i)));
        BOOST_TEST(i == 6);
        BOOST_TEST((!test_attr("XXX", sym, i)));
    }

    { // allow std::string and other string types
        symbols<> sym;

        // const and non-const std::string
        std::string a("abc");
        std::string const b("def");
        sym += a;
        sym += b;
        BOOST_TEST((test("abc", sym)));
        BOOST_TEST((test("def", sym)));
        sym = a;
        BOOST_TEST((test("abc", sym)));
        BOOST_TEST((!test("def", sym)));

        // non-const C-style string
        char arr[2]; arr[0] = 'a'; arr[1] = '\0';
        sym = arr;
        BOOST_TEST((test("a", sym)));
        BOOST_TEST((!test("b", sym)));

        // const and non-const custom string type
        custom_string_c c('x');
        custom_string_c const cc('y');
        sym = c, cc;
        BOOST_TEST((test("x", sym)));
        BOOST_TEST((test("y", sym)));
        BOOST_TEST((!test("z", sym)));
    }

    {
        namespace phx = boost::phoenix;

        symbols<char, int> sym;
        sym.add
            ("a", 1)
            ("b", 2)
        ;

        rule<char const*, int(symbols<char, int>&)> r;
        r %= lazy(_r1);

        int i = 0;
        BOOST_TEST(test_attr("a", r(phx::ref(sym)), i));
        BOOST_TEST(i == 1);
        BOOST_TEST(test_attr("b", r(phx::ref(sym)), i));
        BOOST_TEST(i == 2);
        BOOST_TEST(!test("c", r(phx::ref(sym))));
    }

    { // find

        symbols<char, int> sym;
        sym.add("a", 1)("b", 2);

        BOOST_TEST(!sym.find("c"));

        BOOST_TEST(sym.find("a") && *sym.find("a") == 1);
        BOOST_TEST(sym.find("b") && *sym.find("b") == 2);

        BOOST_TEST(sym.at("a") == 1);
        BOOST_TEST(sym.at("b") == 2);
        BOOST_TEST(sym.at("c") == 0);

        BOOST_TEST(sym.find("a") && *sym.find("a") == 1);
        BOOST_TEST(sym.find("b") && *sym.find("b") == 2);
        BOOST_TEST(sym.find("c") && *sym.find("c") == 0);

        symbols<char, int> const_sym(sym);

        BOOST_TEST(const_sym.find("a") && *const_sym.find("a") == 1);
        BOOST_TEST(const_sym.find("b") && *const_sym.find("b") == 2);
        BOOST_TEST(const_sym.find("c") && *const_sym.find("c") == 0);
        BOOST_TEST(!const_sym.find("d"));

        char const *str1 = "all";
        char const *first = str1, *last = str1 + 3;
        BOOST_TEST(*sym.prefix_find(first, last) == 1 && first == str1 + 1);

        char const *str2 = "dart";
        first = str2; last = str2 + 4;
        BOOST_TEST(!sym.prefix_find(first, last) && first == str2);
    }

    { // name
        symbols <char, int> sym,sym2;
        sym.name("test");
        BOOST_TEST(sym.name()=="test");
        sym2 = sym;
        BOOST_TEST(sym2.name()=="test");

        symbols <char,int> sym3(sym);
        BOOST_TEST(sym3.name()=="test");
    }

    { // Substrings

        symbols<char, int> sym;
        BOOST_TEST(sym.at("foo") == 0);
        sym.at("foo") = 1;
        BOOST_TEST(sym.at("foo") == 1);
        BOOST_TEST(sym.at("fool") == 0);
        sym.at("fool") = 2;
        BOOST_TEST(sym.find("foo") && *sym.find("foo") == 1);
        BOOST_TEST(sym.find("fool") && *sym.find("fool") == 2);
        BOOST_TEST(!sym.find("foolish"));
        BOOST_TEST(!sym.find("foot"));
        BOOST_TEST(!sym.find("afoot"));

        char const *str, *first, *last;
        str = "foolish"; first = str; last = str + 7;
        BOOST_TEST(*sym.prefix_find(first, last) == 2 && first == str + 4);

        first = str; last = str + 4;
        BOOST_TEST(*sym.prefix_find(first, last) == 2 && first == str + 4);

        str = "food"; first = str; last = str + 4;
        BOOST_TEST(*sym.prefix_find(first, last) == 1 && first == str + 3);

        first = str; last = str + 3;
        BOOST_TEST(*sym.prefix_find(first, last) == 1 && first == str + 3);

        first = str; last = str + 2;
        BOOST_TEST(!sym.prefix_find(first, last) && first == str);
    }

    {
        // remove bug

        std::string s;
        symbols<char, double> vars;

        vars.add("l1", 12.0);
        vars.add("l2", 0.0);
        vars.remove("l2");
        vars.find("l1");
        double* d = vars.find("l1");
        BOOST_TEST(d != 0);
    }

    { // test for proto problem with rvalue references (10-11-2011)
        symbols<char, int> sym;
        sym += get_str("Joel");
        sym += get_str("Ruby");

        BOOST_TEST((test("Joel", sym)));
        BOOST_TEST((test("Ruby", sym)));
    }

    return boost::report_errors();
}
