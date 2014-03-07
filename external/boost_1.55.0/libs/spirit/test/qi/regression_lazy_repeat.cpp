/*=============================================================================
    Copyright (c) 2011      Bryce Lelbach

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_repeat.hpp>
#include <boost/spirit/include/phoenix_core.hpp>

#include "test.hpp"

int
main()
{
    using spirit_test::test_attr;
    using boost::spirit::qi::repeat;
    using boost::spirit::qi::char_;
    using boost::phoenix::ref;

    int n = 5;
    std::string s = "";

    // this was broken by the addition of handles_container, due to incorrect
    // dispatching of lazy parsers/directives/terminals in pass_container
    BOOST_TEST(test_attr("foobar", char_ >> repeat(ref(n))[char_], s));

    BOOST_TEST_EQ(s, "foobar");

    return boost::report_errors();
}
  
