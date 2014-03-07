//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/karma_auxiliary.hpp>
#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_generate.hpp>

#include <iostream>
#include "test.hpp"

int
main()
{
    using namespace spirit_test;
    using namespace boost::spirit;
    using namespace boost::spirit::ascii;

    {
        BOOST_TEST((test("\n", eol)));
        BOOST_TEST((test("\n", eol)));
    }

    {
        BOOST_TEST((test_delimited("\n ", eol, space)));
        BOOST_TEST((test_delimited("\n ", eol, space)));
    }

    return boost::report_errors();
}
