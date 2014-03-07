/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/qi_nonterminal.hpp>
#include <boost/spirit/include/support_argument.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/optional.hpp>

#include <string>
#include <iostream>
#include "test.hpp"

using namespace spirit_test;

int
main()
{
    using boost::spirit::qi::int_;
    using boost::spirit::qi::_1;
    using boost::spirit::qi::_2;
    using boost::spirit::qi::rule;
    using boost::spirit::ascii::alpha;
    using boost::spirit::ascii::char_;

    using boost::fusion::vector;
    using boost::fusion::at_c;
    using boost::optional;

    {
        BOOST_TEST((test("a", char_('a') ^ char_('b') ^ char_('c'))));
        BOOST_TEST((test("b", char_('a') ^ char_('b') ^ char_('c'))));
        BOOST_TEST((test("ab", char_('a') ^ char_('b') ^ char_('c'))));
        BOOST_TEST((test("ba", char_('a') ^ char_('b') ^ char_('c'))));
        BOOST_TEST((test("abc", char_('a') ^ char_('b') ^ char_('c'))));
        BOOST_TEST((test("acb", char_('a') ^ char_('b') ^ char_('c'))));
        BOOST_TEST((test("bca", char_('a') ^ char_('b') ^ char_('c'))));
        BOOST_TEST((test("bac", char_('a') ^ char_('b') ^ char_('c'))));
        BOOST_TEST((test("cab", char_('a') ^ char_('b') ^ char_('c'))));
        BOOST_TEST((test("cba", char_('a') ^ char_('b') ^ char_('c'))));

        BOOST_TEST((!test("cca", char_('a') ^ char_('b') ^ char_('c'))));
    }

    {
        vector<optional<int>, optional<char> > attr;

        BOOST_TEST((test_attr("a", int_ ^ alpha, attr)));
        BOOST_TEST((!at_c<0>(attr)));
        BOOST_TEST((at_c<1>(attr).get() == 'a'));

        at_c<1>(attr) = optional<char>(); // clear the optional
        BOOST_TEST((test_attr("123", int_ ^ alpha, attr)));
        BOOST_TEST((at_c<0>(attr).get() == 123));
        BOOST_TEST((!at_c<1>(attr)));

        at_c<0>(attr) = optional<int>(); // clear the optional
        BOOST_TEST((test_attr("123a", int_ ^ alpha, attr)));
        BOOST_TEST((at_c<0>(attr).get() == 123));
        BOOST_TEST((at_c<1>(attr).get() == 'a'));

        at_c<0>(attr) = optional<int>(); // clear the optional
        at_c<1>(attr) = optional<char>(); // clear the optional
        BOOST_TEST((test_attr("a123", int_ ^ alpha, attr)));
        BOOST_TEST((at_c<0>(attr).get() == 123));
        BOOST_TEST((at_c<1>(attr).get() == 'a'));
    }

    {   // test action
        using namespace boost::phoenix;
        namespace phx = boost::phoenix;

        optional<int> i;
        optional<char> c;

        BOOST_TEST((test("123a", (int_ ^ alpha)[phx::ref(i) = _1, phx::ref(c) = _2])));
        BOOST_TEST((i.get() == 123));
        BOOST_TEST((c.get() == 'a'));
    }

    {   // test rule %=

        typedef vector<optional<int>, optional<char> > attr_type;
        attr_type attr;

        rule<char const*, attr_type()> r;
        r %= int_ ^ alpha;

        BOOST_TEST((test_attr("a", r, attr)));
        BOOST_TEST((!at_c<0>(attr)));
        BOOST_TEST((at_c<1>(attr).get() == 'a'));

        at_c<1>(attr) = optional<char>(); // clear the optional
        BOOST_TEST((test_attr("123", r, attr)));
        BOOST_TEST((at_c<0>(attr).get() == 123));
        BOOST_TEST((!at_c<1>(attr)));

        at_c<0>(attr) = optional<int>(); // clear the optional
        BOOST_TEST((test_attr("123a", r, attr)));
        BOOST_TEST((at_c<0>(attr).get() == 123));
        BOOST_TEST((at_c<1>(attr).get() == 'a'));

        at_c<0>(attr) = optional<int>(); // clear the optional
        at_c<1>(attr) = optional<char>(); // clear the optional
        BOOST_TEST((test_attr("a123", r, attr)));
        BOOST_TEST((at_c<0>(attr).get() == 123));
        BOOST_TEST((at_c<1>(attr).get() == 'a'));
    }

    return boost::report_errors();
}

