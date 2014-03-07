//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_operator.hpp>
#include <boost/spirit/include/karma_directive.hpp>
#include <boost/spirit/include/karma_char.hpp>

#include <iostream>
#include "test.hpp"

int
main()
{
    using namespace spirit_test;
    using namespace boost::spirit;

    {
        using boost::spirit::karma::double_;
        using boost::spirit::karma::buffer;

        std::vector<double> v;
        BOOST_TEST(test("", -buffer['[' << +double_ << ']'], v));

        v.push_back(1.0);
        v.push_back(2.0);
        BOOST_TEST(test("[1.02.0]", buffer['[' << +double_ << ']'], v));
        BOOST_TEST(test("[1.02.0]", buffer[buffer['[' << +double_ << ']']], v));
    }

    {
        using boost::spirit::karma::double_;
        using boost::spirit::karma::buffer;
        using boost::spirit::ascii::space;

        std::vector<double> v;
        BOOST_TEST(test_delimited("", 
            -buffer['[' << +double_ << ']'], v, space));

        v.push_back(1.0);
        v.push_back(2.0);
        BOOST_TEST(test_delimited("[ 1.0 2.0 ] ", 
            buffer['[' << +double_ << ']'], v, space));
    }

    return boost::report_errors();
}
