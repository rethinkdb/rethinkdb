/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi_directive.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_int.hpp>
#include <boost/spirit/include/qi_operator.hpp>

#include <iostream>
#include <vector>
#include "test.hpp"

int
main()
{
    using spirit_test::test;
    using spirit_test::test_attr;
    using boost::spirit::qi::hold;
    using boost::spirit::qi::int_;
    using boost::spirit::ascii::alpha;

    {
        std::vector<int> vec;
        BOOST_TEST(!test_attr("1$", hold[int_ >> ';'], vec));
        BOOST_TEST(vec.size() == 0);
        BOOST_TEST(test_attr("1;", hold[int_ >> ';'], vec));
        BOOST_TEST(vec.size() == 1);
        BOOST_TEST(vec[0] == 1);
    }

    {
        std::string attr;
        BOOST_TEST(
            test_attr(
                "abc;",
                hold[alpha >> ';'] | (+alpha >> ';'),
                attr));
        BOOST_TEST(attr == "abc");
    }

    return boost::report_errors();
}
