//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/karma_auxiliary.hpp>
#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_operator.hpp>
#include <boost/spirit/include/phoenix_core.hpp>

#include <iostream>
#include "test.hpp"

int
main()
{
    using namespace spirit_test;
    using namespace boost::spirit;

    {
        using namespace boost::spirit::ascii;

        BOOST_TEST(test("", eps));
        BOOST_TEST(test_delimited(" ", eps, space));

        BOOST_TEST(!test("", !eps));
        BOOST_TEST(!test_delimited(" ", !eps, space));
    }

    {   // test direct argument
        using namespace boost::phoenix;

        BOOST_TEST(test("", eps(true)));
        BOOST_TEST(!test("", eps(false)));
    }

    {   // test action
        using namespace boost::phoenix;

        BOOST_TEST(test("", eps(val(true))));
        BOOST_TEST(!test("", eps(val(false))));
    }

    {   // test no delimiter when argument is false
        using namespace boost::spirit::ascii;

        std::string generated;
        std::back_insert_iterator<std::string> outit(generated);
        BOOST_TEST(!karma::generate_delimited(outit, eps(false), space));
        BOOST_TEST(generated.empty());
    }

    return boost::report_errors();
}
