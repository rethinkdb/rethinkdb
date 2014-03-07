/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_action.hpp>
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

int
main()
{
    using spirit_test::test;
    using spirit_test::test_attr;

    using boost::spirit::qi::int_;
    using boost::spirit::qi::_1;
    using boost::spirit::qi::_2;
    using boost::spirit::ascii::char_;
    using boost::spirit::ascii::alpha;
    using boost::fusion::vector;
    using boost::fusion::at_c;
    using boost::optional;

    {
        BOOST_TEST((test("a", char_('a') || char_('b'))));
        BOOST_TEST((test("b", char_('a') || char_('b'))));
        BOOST_TEST((test("ab", char_('a') || char_('b'))));
    }

    {
        vector<optional<int>, optional<char> > attr;

        BOOST_TEST((test_attr("a", int_ || alpha, attr)));
        BOOST_TEST((!at_c<0>(attr)));
        BOOST_TEST((at_c<1>(attr).get() == 'a'));

        at_c<1>(attr) = optional<char>(); // clear the optional
        BOOST_TEST((test_attr("123", int_ || alpha, attr)));
        BOOST_TEST((at_c<0>(attr).get() == 123));
        BOOST_TEST((!at_c<1>(attr)));

        at_c<0>(attr) = optional<int>(); // clear the optional
        BOOST_TEST((test_attr("123a", int_ || alpha, attr)));
        BOOST_TEST((at_c<0>(attr).get() == 123));
        BOOST_TEST((at_c<1>(attr).get() == 'a'));

        BOOST_TEST((!test("a123", int_ || alpha)));
    }

    {   // test whether optional<optional<>> gets properly handled 
        vector<optional<int>, optional<int> > attr1;
        BOOST_TEST((test_attr("123", int_ || '[' >> -int_ >> ']', attr1)));
        BOOST_TEST((at_c<0>(attr1) && at_c<0>(attr1).get() == 123));
        BOOST_TEST((!at_c<1>(attr1)));

        vector<optional<int>, optional<int> > attr2;
        BOOST_TEST((test_attr("[123]", int_ || '[' >> -int_ >> ']', attr2)));
        BOOST_TEST((!at_c<0>(attr2)));
        BOOST_TEST((at_c<1>(attr2) && at_c<1>(attr2).get() == 123));

        vector<optional<int>, optional<optional<int> > > attr3;
        BOOST_TEST((test_attr("[]", int_ || '[' >> -int_ >> ']', attr3)));
        BOOST_TEST((!at_c<0>(attr3)));
        BOOST_TEST((at_c<1>(attr3) && !at_c<1>(attr3).get()));
    }

    {   // test unused attribute handling

        vector<optional<int>, optional<char> > attr;
        BOOST_TEST((test_attr("123abc", int_ || ("ab" >> char_), attr)));
        BOOST_TEST((at_c<0>(attr).get() == 123));
        BOOST_TEST((at_c<1>(attr).get() == 'c'));
    }

    {   // test unused attribute handling

        optional<int> attr;
        BOOST_TEST((test_attr("123ab", int_ || "ab", attr)));
        BOOST_TEST((attr == 123));
    }

    {   // test action
        namespace phx = boost::phoenix;

        optional<int> i;
        optional<char> c;

        BOOST_TEST((test("123a", (int_ || alpha)[phx::ref(i) = _1, phx::ref(c) = _2])));
        BOOST_TEST((i.get() == 123));
        BOOST_TEST((c.get() == 'a'));
    }

    return boost::report_errors();
}

