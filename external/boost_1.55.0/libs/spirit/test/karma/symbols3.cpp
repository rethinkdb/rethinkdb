//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/karma_auxiliary.hpp>
#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_operator.hpp>
#include <boost/spirit/include/karma_directive.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_nonterminal.hpp>

#include "test.hpp"

namespace fusion = boost::fusion;

template <typename T>
inline std::vector<T> 
make_vector(T const& t1, T const& t2)
{
    std::vector<T> v;
    v.push_back(t1);
    v.push_back(t2);
    return v;
}

int main()
{
    using spirit_test::test;
    using boost::spirit::karma::symbols;

    { // more advanced
        using boost::spirit::karma::rule;
        using boost::spirit::karma::lit;
        using boost::spirit::karma::char_;

        typedef spirit_test::output_iterator<char>::type output_iterator_type;

        symbols<char, rule<output_iterator_type, char()> > sym;
        rule<output_iterator_type, char()> r1 = char_;
        
        sym.add
            ('j', r1.alias())
            ('h', r1.alias())
            ('t', r1.alias())
            ('k', r1.alias())
        ;

        boost::mpl::true_ f = 
            boost::mpl::bool_<boost::spirit::traits::is_generator<
                symbols<char, rule<output_iterator_type, char()> > >::value>();

        // silence stupid compiler warnings 
        // i.e. MSVC warning C4189: 'f' : local variable is initialized but not referenced
        BOOST_TEST((f.value));

        BOOST_TEST((test("J", sym, make_vector('j', 'J'))));
        BOOST_TEST((test("H", sym, make_vector('h', 'H'))));
        BOOST_TEST((test("T", sym, make_vector('t', 'T'))));
        BOOST_TEST((test("K", sym, make_vector('k', 'K'))));
        BOOST_TEST((!test("", sym, 'x')));

        // test copy
        symbols<char, rule<output_iterator_type, char()> > sym2;
        sym2 = sym;
        BOOST_TEST((test("J", sym2, make_vector('j', 'J'))));
        BOOST_TEST((test("H", sym2, make_vector('h', 'H'))));
        BOOST_TEST((test("T", sym2, make_vector('t', 'T'))));
        BOOST_TEST((test("K", sym2, make_vector('k', 'K'))));
        BOOST_TEST((!test("", sym2, 'x')));

        // make sure it plays well with other generators
        BOOST_TEST((test("Jyo", sym << "yo", make_vector('j', 'J'))));

        sym.remove
            ('j')
            ('h')
        ;

        BOOST_TEST((!test("", sym, 'j')));
        BOOST_TEST((!test("", sym, 'h')));
    }

    { // basics
        symbols<std::string> sym;

        sym.add
            ("Joel")
            ("Hartmut")
            ("Tom")
            ("Kim")
        ;

        boost::mpl::true_ f = 
            boost::mpl::bool_<boost::spirit::traits::is_generator<
                symbols<char, std::string> >::value>();

        // silence stupid compiler warnings 
        // i.e. MSVC warning C4189: 'f' : local variable is initialized but not referenced
        BOOST_TEST((f.value));

        BOOST_TEST((test("Joel", sym, "Joel")));
        BOOST_TEST((test("Hartmut", sym, "Hartmut")));
        BOOST_TEST((test("Tom", sym, "Tom")));
        BOOST_TEST((test("Kim", sym, "Kim")));
        BOOST_TEST((!test("", sym, "X")));

        // test copy
        symbols<std::string> sym2;
        sym2 = sym;
        BOOST_TEST((test("Joel", sym2, "Joel")));
        BOOST_TEST((test("Hartmut", sym2, "Hartmut")));
        BOOST_TEST((test("Tom", sym2, "Tom")));
        BOOST_TEST((test("Kim", sym2, "Kim")));
        BOOST_TEST((!test("", sym2, "X")));

        // make sure it plays well with other generators
        BOOST_TEST((test("Joelyo", sym << "yo", "Joel")));

        sym.remove
            ("Joel")
            ("Hartmut")
        ;

        BOOST_TEST((!test("", sym, "Joel")));
        BOOST_TEST((!test("", sym, "Hartmut")));
    }

    { // name
        symbols <std::string> sym("test1"), sym2;
        BOOST_TEST(sym.name() == "test1");

        sym.name("test");
        BOOST_TEST(sym.name() == "test");
        sym2 = sym;
        BOOST_TEST(sym2.name() == "test");

        symbols <std::string> sym3(sym);
        BOOST_TEST(sym3.name() == "test");
    }

    return boost::report_errors();
}
