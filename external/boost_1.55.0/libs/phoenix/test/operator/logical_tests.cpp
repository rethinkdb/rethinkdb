/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>

namespace phoenix = boost::phoenix;

int
main()
{
    using phoenix::arg_names::arg1;
    using phoenix::arg_names::arg2;
    {
        bool x = false;
        bool y = true;

        BOOST_TEST((!arg1)(x) == true);
        BOOST_TEST((!arg1)(y) == false);
        BOOST_TEST((arg1 || arg2)(x, y) == true);
        BOOST_TEST((arg1 && arg2)(x, y) == false);

        // short circuiting:
        int i = 1234;
        (arg1 || (arg2 = 4567))(y, i);
        BOOST_TEST(i == 1234);
        (arg1 && (arg2 = 4567))(y, i);
        BOOST_TEST(i == 4567);
    }

    return boost::report_errors();
}
