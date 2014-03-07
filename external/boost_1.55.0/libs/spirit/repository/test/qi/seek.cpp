/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2011 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/


#include <vector>

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/qi_parse.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/qi_int.hpp>
#include <boost/spirit/include/qi_sequence.hpp>
#include <boost/spirit/include/qi_plus.hpp>
#include <boost/spirit/include/qi_eoi.hpp>
#include <boost/spirit/include/qi_action.hpp>

#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#include <boost/spirit/repository/include/qi_seek.hpp>

#include "test.hpp"


///////////////////////////////////////////////////////////////////////////////
int main()
{
    using namespace spirit_test;
    namespace qi = boost::spirit::qi;
    namespace phx = boost::phoenix;
    using boost::spirit::repository::qi::seek;
    using boost::spirit::standard::space;

    // test eoi
    {
        using qi::eoi;

        BOOST_TEST(test("", seek[eoi]));
        BOOST_TEST(test(" ", seek[eoi], space));
        BOOST_TEST(test("a", seek[eoi]));
        BOOST_TEST(test(" a", seek[eoi], space));
    }

    // test literal finding
    {
        using qi::int_;
        using qi::char_;

        int i = 0;

        BOOST_TEST(
            test_attr("!@#$%^&*KEY:123", seek["KEY:"] >> int_, i)
            && i == 123
        );
    }

    // test sequence finding
    {
        using qi::int_;
        using qi::lit;

        int i = 0;

        BOOST_TEST(
            test_attr("!@#$%^&* KEY : 123", seek[lit("KEY") >> ':'] >> int_, i, space)
            && i == 123
        );
    }

    // test attr finding
    {
        using qi::int_;

        std::vector<int> v;

        BOOST_TEST( // expect partial match
            test_attr("a06b78c3d", +seek[int_], v, false)
            && v[0] == 6 && v[1] == 78 && v[2] == 3
        );
    }

    // test action
    {
        using phx::ref;

        bool b = false;

        BOOST_TEST( // expect partial match
            test("abcdefg", seek["def"][ref(b) = true], false)
            && b
        );
    }

    return boost::report_errors();
}
